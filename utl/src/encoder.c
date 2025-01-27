#include "encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"
#include "builtins.h"

char* utl_EncodeBuf_alloc(utl_EncodeBuf* buf, const size_t n_bytes) {
    if(buf->pos + n_bytes > buf->size) {
        buf->size = (buf->pos + n_bytes) * 1.25;
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

void utl_encode_int64(int64_t value, utl_EncodeBuf* buf) {
    utl_encode_intX((char*)&value, buf, 8);
}

void utl_encode_double(double value, utl_EncodeBuf* buf) {
    utl_encode_int64(*(uint64_t*)&value, buf);
}

void utl_encode_bool(const bool value, utl_EncodeBuf* buf) {
    char* tmp = utl_EncodeBuf_alloc(buf, 4);
    memcpy(tmp, value ? BOOL_TRUE : BOOL_FALSE, 4);
}

void utl_encode_bytes(const utl_StringView value, utl_EncodeBuf* buf) {
    char* tmp;
    size_t total_size = value.size;
    if(value.size >= 254) {
        tmp = utl_EncodeBuf_alloc(buf, 4);
        tmp[0] = 254;
        tmp[1] = value.size & 0xFF;
        tmp[2] = (value.size >> 8) & 0xFF;
        tmp[3] = (value.size >> 16) & 0xFF;
    } else {
        tmp = utl_EncodeBuf_alloc(buf, 1);
        tmp[0] = value.size;
        ++total_size;
    }

    tmp = utl_EncodeBuf_alloc(buf, value.size);
    memcpy(tmp, value.data, value.size);
    uint8_t padding = total_size % 4;
    if(padding) {
        padding = 4 - padding;
        tmp = utl_EncodeBuf_alloc(buf, padding);
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
            utl_encode_bool(*(bool*)value, buf);
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
            utl_encode_internal(*(utl_Message**)value, buf);
            break;
        }
        case VECTOR: {
            const utl_Vector* vector = *(utl_Vector**)value;
            utl_encode_int32(VECTOR_CONSTR, buf);
            utl_encode_int32(utl_Vector_size(vector), buf);
            for(size_t i = 0; i < utl_Vector_size(vector); i++) {
                utl_encode_field((utl_FieldDef*)vector->message_def, utl_Vector_rawValue(vector, i), buf, vector->message_def->element_size);
            }
            break;
        }
    }
}

void utl_encode_internal(const utl_Message* message, utl_EncodeBuf* buf) {
    utl_encode_intX((char*)&message->message_def->id, buf, 4);

    const utl_FieldDef* fields = message->message_def->fields;
    for(int i = 0; i < message->message_def->fields_num; i++) {
        const utl_FieldDef field = fields[i];
        if(field.flag_info && field.type != FLAGS && !utl_Message_hasField(message, &field))
            continue;

        if(field.type != BIT_BOOL) {
            const size_t next_offset = (i == (message->message_def->fields_num - 1)) ? message->message_def->size : message->message_def->fields[i + 1].offset;
            const size_t item_size = next_offset - field.offset;
            void* value = message->data + field.offset;
            utl_encode_field(&field, value, buf, item_size);
        }
    }
}

char* utl_encode(const utl_Message* message, size_t* out_size) {
    const size_t alloc_size = 4 + message->message_def->fields_num * 8;
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