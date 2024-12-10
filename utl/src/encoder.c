#include "encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"
#include "builtins.h"
#ifndef UTL_DONT_USE_COMPILE_TIME_ENDIANNESS_CHECK
#include "endianness.h"
#endif

char* utl_EncodeBuf_alloc(utl_EncodeBuf* buf, const size_t n_bytes) {
    if(buf->pos + n_bytes > buf->size) {
        buf->size = (buf->pos + n_bytes) * 1.25;
        buf->data = realloc(buf->data, buf->size);
    }

    char* result = buf->data + buf->pos;
    buf->pos += n_bytes;

    return result;
}

void utl_encode_intX(char* value, utl_EncodeBuf* buf, uint8_t bytes_size) {
    char* tmp = utl_EncodeBuf_alloc(buf, bytes_size);
#ifndef UTL_DONT_USE_COMPILE_TIME_ENDIANNESS_CHECK
#ifdef __BIG_ENDIAN__
    for(int i = 0; i < bytes_size; i++) {
        buf[i] = value[bytes_size-i-1];
    }
#else
    memcpy(tmp, value, bytes_size);
#endif
#else
    if(is_big_endian()) {
        for(int i = 0; i < bytes_size; i++) {
            buf[i] = value[bytes_size-i-1];
        }
    } else {
        memcpy(buf, value, bytes_size);
    }
#endif
}

void utl_encode_int32(int32_t value, utl_EncodeBuf* buf) {
    utl_encode_intX((char*)&value, buf, 4);
}

void utl_encode_int64(int64_t value, utl_EncodeBuf* buf) {
    utl_encode_intX((char*)&value, buf, 8);
}

void utl_encode_int128(char value[16], utl_EncodeBuf* buf) {
    char* tmp = utl_EncodeBuf_alloc(buf, 16);
    memcpy(tmp, value, 16);
}

void utl_encode_int256(char value[32], utl_EncodeBuf* buf) {
    char* tmp = utl_EncodeBuf_alloc(buf, 32);
    memcpy(tmp, value, 32);
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

void utl_encode_field(const utl_FieldDef* field, void* value, utl_EncodeBuf* buf) {
    const utl_FieldType field_type = field->type;

    switch (field_type) {
        case FLAGS:
        case INT32: {
            utl_encode_int32(((utl_Int32*)value)->value, buf);
            break;
        }
        case INT64: {
            utl_encode_int64(((utl_Int64*)value)->value, buf);
            break;
        }
        case INT128: {
            utl_encode_int128(((utl_Int128*)value)->value, buf);
            break;
        }
        case INT256: {
            utl_encode_int256(((utl_Int256*)value)->value, buf);
            break;
        }
        case DOUBLE: {
            utl_encode_double(((utl_Double*)value)->value, buf);
            break;
        }
        case FULL_BOOL: {
            utl_encode_bool(((utl_Bool*)value)->value, buf);
            break;
        }
        case BIT_BOOL: {
            break;
        }
        case BYTES:
        case STRING: {
            utl_encode_bytes(((utl_Bytes*)value)->value, buf);
            break;
        }
        case TLOBJECT: {
            utl_encode_internal(value, buf);
            break;
        }
        case VECTOR: {
            const utl_Vector* vector = value;
            utl_encode_int32(VECTOR_CONSTR, buf);
            utl_encode_int32(utl_Vector_size(vector), buf);
            for(size_t i = 0; i < utl_Vector_size(vector); i++) {
                utl_encode_field((utl_FieldDef*)vector->message_def, utl_Vector_value(vector, i), buf);
            }
            break;
        }
    }
}

void utl_encode_internal(const utl_Message* message, utl_EncodeBuf* buf) {
    size_t* flags = NULL;
    if (message->message_def->flags_num) {
        flags = malloc(message->message_def->flags_num * sizeof(size_t));
    }

    utl_encode_intX((char*)&message->message_def->id, buf, 4);

    const utl_FieldDef* fields = message->message_def->fields;
    for(int i = 0; i < message->message_def->fields_num; i++) {
        const utl_FieldDef field = fields[i];
        void* value = message->table[field.num];
        if(field.type == FLAGS) {
            flags[(field.flag_info >> 5) - 1] = buf->pos;
            ((utl_Int32*)value)->value = 0;
        }

        if(value == NULL) {
            // TODO: check if optional, if not - fail
            continue;
        }

        if(field.type != FLAGS && field.flag_info && !(field.type == BIT_BOOL && !((utl_Bool*)value)->value)) {
            uint8_t flag_bit = (field.flag_info & 0b11111);
#ifndef UTL_DONT_USE_COMPILE_TIME_ENDIANNESS_CHECK
#ifdef __BIG_ENDIAN__
            flag_bit = 31 - flag_bit;
#endif
#else
            if(is_big_endian())
                flag_bit = 31 - flag_bit;
#endif
            const uint32_t flag_value = 1 << flag_bit;
            *(uint32_t*)(buf->data + flags[(field.flag_info >> 5) - 1]) |= flag_value;
        }

        if(field.type != BIT_BOOL)
            utl_encode_field(&field, value, buf);
    }

    if(flags)
        free(flags);
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