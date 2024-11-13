#include "decoder.h"
#include "encoder.h"

#include <string.h>

char* utl_DecodeBuf_read(utl_DecodeBuf* buf, size_t n) {
    if(buf->pos >= buf->size)
        return NULL;
    char* ptr = buf->data + buf->pos;
    buf->pos += n;
    return ptr;
}

void utl_decode_intX(char* value, utl_DecodeBuf* buffer, uint8_t bytes_size) {
    const char* buf = utl_DecodeBuf_read(buffer, bytes_size);
    if(is_big_endian()) {
        for(int i = 0; i < bytes_size; i++) {
            value[i] = buf[bytes_size-i-1];
        }
    } else {
        memcpy(value, buf, bytes_size);
    }
}

int32_t utl_decode_int32(utl_DecodeBuf* buffer) {
    int32_t result;
    utl_decode_intX((char*)&result, buffer, 4);
    return result;
}

int64_t utl_decode_int64(utl_DecodeBuf* buffer) {
    int64_t result;
    utl_decode_intX((char*)&result, buffer, 8);
    return result;
}

void utl_decode_int128(char* out, utl_DecodeBuf* buffer) {
    char* buf = utl_DecodeBuf_read(buffer, 16);
    memcpy(out, buf, 16);
}

void utl_decode_int256(char* out, utl_DecodeBuf* buffer) {
    char* buf = utl_DecodeBuf_read(buffer, 32);
    memcpy(out, buf, 32);
}

double utl_decode_double(utl_DecodeBuf* buffer) {
    int64_t tmp = utl_decode_int64(buffer);
    return *((double*)&tmp);
}

bool utl_decode_bool(utl_DecodeBuf* buffer) {
    char* buf = utl_DecodeBuf_read(buffer, 4);
    return !memcmp(buf, BOOL_TRUE, 4);
}

utl_StringView utl_decode_bytes(utl_DecodeBuf* buffer, arena_t* arena) {
    char* buf = utl_DecodeBuf_read(buffer, 1);
    uint32_t count = (uint8_t)buf[0];
    uint8_t offset = 1;
    if(buf[0] >= 254) {
        buf = utl_DecodeBuf_read(buffer, 3);
        count = (uint8_t)buf[0] + ((uint8_t)buf[1] << 8) + ((uint8_t)buf[2] << 16);
        offset = 0;
    }

    utl_StringView result = utl_StringView_new(arena, count);
    memcpy(result.data, utl_DecodeBuf_read(buffer, count), count);

    uint32_t padding = (count + offset) % 4;
    if(padding) {
        buffer->pos += 4 - padding;
    }

    return result;
}

void utl_decode_field(utl_Message* message, utl_FieldDef* field, utl_DecodeBuf* buf) {
    switch (field->type) {
        case INT32: {
            utl_Message_setInt32(message, field, utl_decode_int32(buf));
            break;
        }
        case INT64: {
            utl_Message_setInt64(message, field, utl_decode_int64(buf));
            break;
        }
        case INT128: {
            char tmp[16];
            utl_decode_int128(tmp, buf);
            utl_Message_setInt128(message, field, tmp);
            break;
        }
        case INT256: {
            char tmp[32];
            utl_decode_int256(tmp, buf);
            utl_Message_setInt256(message, field, tmp);
            break;
        }
        case DOUBLE: {
            utl_Message_setDouble(message, field, utl_decode_double(buf));
            break;
        }
        case BOOL: {
            utl_Message_setBool(message, field, utl_decode_bool(buf));
            break;
        }
        case BYTES: {
            utl_Message_setBytes(message, field, utl_decode_bytes(buf, &message->arena));
            break;
        }
        case STRING: {
            utl_Message_setString(message, field, utl_decode_bytes(buf, &message->arena));
            break;
        }
        case TLOBJECT: {
            utl_Message* new_message = utl_Message_new(&message->arena, field->sub_message_def);
            utl_Message_setMessage(message, field, new_message);
            buf->pos += utl_decode(new_message, buf->data + buf->pos, buf->size - buf->pos);
            break;
        }
        case VECTOR: {
            // TODO: read vector
            break;
        }
    }
}

size_t utl_decode(utl_Message* message, char* buf, size_t size) {
    utl_DecodeBuf buffer = {
        .data = buf,
        .pos = 0,
        .size = size,
    };

    const utl_FieldDef* fields = message->message_def->fields;
    for(int i = 0; i < message->message_def->fields_num; i++) {
        utl_FieldDef field = fields[i];
        utl_decode_field(message, &field, &buffer);
    }

    return buffer.pos;
}