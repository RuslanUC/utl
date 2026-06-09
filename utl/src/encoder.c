#include "encoder.h"

#include <stdlib.h>
#include <string.h>
#include "vector.h"
#include "logging.h"

char* utl_EncodeBuf_alloc(utl_EncodeBuf* buf, const size_t n_bytes) {
    if(buf->pos + n_bytes > buf->size) {
        _UTL_LOG("[enc] Realloc %zu -> %zu", buf->size, (buf->pos + n_bytes) * 2);
        buf->size = (buf->pos + n_bytes) * 2;
        buf->data = realloc(buf->data, buf->size);
    }

    char* result = buf->data + buf->pos;
    buf->pos += n_bytes;

    return result;
}

void utl_encode_intX(const char* value, utl_EncodeBuf* buf, const uint8_t bytes_size) {
    char* tmp = utl_EncodeBuf_alloc(buf, bytes_size);
    memcpy(tmp, value, bytes_size);
}

void utl_encode_int32(int32_t value, utl_EncodeBuf* buf) {
    utl_encode_intX((char*)&value, buf, 4);
}

void utl_encode_uint32(uint32_t value, utl_EncodeBuf* buf) {
    utl_encode_intX((char*)&value, buf, 4);
}

void utl_encode_int64(int64_t value, utl_EncodeBuf* buf) {
    utl_encode_intX((char*)&value, buf, 8);
}

void utl_encode_double(double value, utl_EncodeBuf* buf) {
    utl_encode_int64(*(int64_t*)&value, buf);
}

void utl_encode_bool(const bool value, utl_EncodeBuf* buf) {
    char* tmp = utl_EncodeBuf_alloc(buf, 4);
    memcpy(tmp, value ? BOOL_TRUE : BOOL_FALSE, 4);
}

void utl_encode_bytes(const utl_StringView value, utl_EncodeBuf* buf) {
    uint8_t* tmp;
    size_t total_size = value.size;
    if(value.size >= 254) {
        tmp = (uint8_t*)utl_EncodeBuf_alloc(buf, 4);
        tmp[0] = 254;
        tmp[1] = value.size & 0xFF;
        tmp[2] = (value.size >> 8) & 0xFF;
        tmp[3] = (value.size >> 16) & 0xFF;
    } else {
        tmp = (uint8_t*)utl_EncodeBuf_alloc(buf, 1);
        tmp[0] = value.size & 0xFF;
        ++total_size;
    }

    tmp = (uint8_t*)utl_EncodeBuf_alloc(buf, value.size);
    memcpy(tmp, value.data, value.size);
    uint8_t padding = total_size % 4;
    if(padding) {
        padding = 4 - padding;
        tmp = (uint8_t*)utl_EncodeBuf_alloc(buf, padding);
        memset(tmp, 0, padding);
    }
}

void utl_encode_internal(const utl_Message* message, utl_EncodeBuf* buf);

void utl_encode_field(const utl_FieldDef* field, void* value, utl_EncodeBuf* buf, size_t size) {
    const utl_FieldType field_type = field->type;

    switch (field_type) {
        case FLAGS:
        case INT32:
        case INT64:
        case INT128:
        case INT256:
        case DOUBLE: {
            char* tmp = utl_EncodeBuf_alloc(buf, size);
            memcpy(tmp, value, size);
            break;
        }
        case FULL_BOOL: {
            utl_encode_bool((*(uint32_t*)value & 0b10) == 0, buf);
            break;
        }
        case BIT_BOOL: {
            break;
        }
        case BYTES:
        case STRING: {
            utl_encode_bytes(*(utl_StringView*)value, buf);
            break;
        }
        case TLOBJECT: {
            //_UTL_LOG("Serializing tlobject %p", *(utl_Message**)value);
            utl_encode_internal(*(utl_Message**)value, buf);
            break;
        }
        case VECTOR: {
            const utl_Vector* vector = *(utl_Vector**)value;
            const utl_MessageDefVector def = *vector->message_def;
            const int32_t vec_size = utl_Vector_size(vector);

            utl_encode_uint32(VECTOR_CONSTR, buf);
            utl_encode_int32(vec_size, buf);

            if(def.type < STATIC_FIELDS_END) {
                uint8_t* dst = (uint8_t*)utl_EncodeBuf_alloc(buf, vec_size * def.element_size);
                memcpy(dst, vector->data, vec_size * def.element_size);
                return;
            }

            for(int32_t i = 0; i < vec_size; i++) {
                utl_encode_field((utl_FieldDef*)vector->message_def, utl_Vector_rawValue(vector, i), buf, vector->message_def->element_size);
            }
            break;
        }

        case STATIC_FIELDS_END:
        case ALL_FIELDS_END:
            break;
    }
}

void utl_encode_internal(const utl_Message* message, utl_EncodeBuf* buf) {
    const utl_MessageDef def = *message->message_def;

    utl_encode_intX((char*)&def.id, buf, 4);

    if(def.fully_static) {
        uint8_t* dst = (uint8_t*)utl_EncodeBuf_alloc(buf, def.size);
        memcpy(dst, message->data, def.size);
        return;
    }

    const utl_FieldDef* fields = def.fields;
    for(int i = 0; i < def.fields_num; i++) {
        const utl_FieldDef field = fields[i];
        if(field.flag_num && field.type != FLAGS && !utl_Message_hasField(message, &field))
            continue;

        if(field.type != BIT_BOOL) {
            const size_t next_offset = (i == (def.fields_num - 1)) ? def.size : def.fields[i + 1].offset;
            const size_t item_size = next_offset - field.offset;
            void* value = message->data + field.offset;
            utl_encode_field(&field, value, buf, item_size);
        }
    }
}

char* utl_encode(const utl_Message* message, size_t* out_size) {
    size_t alloc_size = 4 + message->message_def->fields_num * 8;
    if(alloc_size < 4096)
        alloc_size = 4096;
    _UTL_LOG("[enc] Alloc %zu", alloc_size);
    utl_EncodeBuf buf = {
        .data = malloc(alloc_size),
        .pos = 0,
        .size = alloc_size,
    };

    utl_encode_internal(message, &buf);

    if(out_size)
        *out_size = buf.pos;
    return buf.data;
}