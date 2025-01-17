#include <string.h>

#include "decoder.h"

#include <stdio.h>

#include "encoder.h"
#include "builtins.h"

char* utl_DecodeBuf_read(utl_DecodeBuf* buf, const size_t n) {
    if(buf->pos >= buf->size)
        return NULL;
    char* ptr = buf->data + buf->pos;
    buf->pos += n;
    return ptr;
}

bool check_not_eof(const utl_DecodeBuf* buf, utl_Status* status, const size_t need_bytes) {
    if(buf->size - buf->pos >= need_bytes)
        return true;
    if(status) {
        status->ok = false;
        strncpy(status->message, "Unexpected EOF", UTL_STATUS_MAX_MESSAGE_SIZE);
    }
    return false;
}

void utl_decode_intX(char* value, utl_DecodeBuf* buffer, const uint8_t bytes_size) {
    const char* buf = utl_DecodeBuf_read(buffer, bytes_size);
    memcpy(value, buf, bytes_size);
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
    const char* buf = utl_DecodeBuf_read(buffer, 16);
    memcpy(out, buf, 16);
}

void utl_decode_int256(char* out, utl_DecodeBuf* buffer) {
    const char* buf = utl_DecodeBuf_read(buffer, 32);
    memcpy(out, buf, 32);
}

double utl_decode_double(utl_DecodeBuf* buffer) {
    int64_t tmp = utl_decode_int64(buffer);
    return *(double*)&tmp;
}

bool utl_decode_bool(utl_DecodeBuf* buffer) {
    const char* buf = utl_DecodeBuf_read(buffer, 4);
    return !memcmp(buf, BOOL_TRUE, 4);
}

utl_StringView emptyStringView = {
    .size = 0,
    .data = NULL,
};

utl_StringView utl_decode_bytes(utl_DecodeBuf* buffer, utl_Arena* arena, utl_Status* status) {
    if(!check_not_eof(buffer, status, 1)) {
        return emptyStringView;
    }
    const char* buf = utl_DecodeBuf_read(buffer, 1);
    uint32_t count = (uint8_t)buf[0];
    uint8_t offset = 1;
    if(count >= 254) {
        if(!check_not_eof(buffer, status, 3)) {
            return emptyStringView;
        }
        buf = utl_DecodeBuf_read(buffer, 3);
        count = (uint8_t)buf[0] + ((uint8_t)buf[1] << 8) + ((uint8_t)buf[2] << 16);
        offset = 0;
    }

    const utl_StringView result = utl_StringView_new(arena, count);
    memcpy(result.data, utl_DecodeBuf_read(buffer, count), count);

    const uint32_t padding = (count + offset) % 4;
    if(padding) {
        buffer->pos += 4 - padding;
    }

    return result;
}

bool utl_decode_vector(utl_Vector* vector, utl_DefPool* def_pool, const utl_MessageDefVector* field, utl_DecodeBuf* buf, const size_t size, utl_Status* status) {
    for(size_t i = 0; i < size; i++) {
        if(!check_not_eof(buf, status, vector->message_def->element_size))
            return false;

        switch (field->type) {
            case FLAGS:
            case BIT_BOOL: {
                break;
            }
            case INT32: {
                utl_Vector_appendInt32(vector, utl_decode_int32(buf));
                break;
            }
            case INT64: {
                utl_Vector_appendInt64(vector, utl_decode_int64(buf));
                break;
            }
            case INT128: {
                utl_Int128 bytes;
                utl_decode_int128(bytes.value, buf);
                utl_Vector_appendInt128(vector, bytes);
                break;
            }
            case INT256: {
                utl_Int256 bytes;
                utl_decode_int256(bytes.value, buf);
                utl_Vector_appendInt256(vector, bytes);
                break;
            }
            case DOUBLE: {
                utl_Vector_appendDouble(vector, utl_decode_double(buf));
                break;
            }
            case FULL_BOOL: {
                if(!check_not_eof(buf, status, 4))
                    return false;
                utl_Vector_appendBool(vector, utl_decode_bool(buf));
                break;
            }
            case BYTES:
            case STRING: {
                const utl_StringView bytes = utl_decode_bytes(buf, &vector->arena, status);
                if(!status)
                    return false;
                field->type == BYTES ? utl_Vector_appendBytes(vector, bytes) : utl_Vector_appendString(vector, bytes);
                break;
            }
            case TLOBJECT: {
                const uint32_t tl_id = utl_decode_int32(buf);
                utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
                if (!new_def) {
                    if(status) {
                        status->ok = false;
                        strncpy(status->message, "Unknown object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                    }
                    return false;
                }

                const utl_TypeDef* type = field->sub.type_def;
                if (type && new_def->type != type) {
                    if(status) {
                        status->ok = false;
                        strncpy(status->message, "Invalid object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                    }
                    return false;
                }

                utl_Message* new_message = utl_Message_new(new_def);
                buf->pos += utl_decode(new_message, def_pool, buf->data + buf->pos, buf->size - buf->pos, status);
                if(!status->ok)
                    return false;
                utl_Vector_appendMessage(vector, new_message);
                break;
            }
            case VECTOR: {
                if (utl_decode_int32(buf) != VECTOR_CONSTR) {
                    if(status) {
                        status->ok = false;
                        strncpy(status->message, "Expected vector id", UTL_STATUS_MAX_MESSAGE_SIZE);
                    }
                    return false;
                }
                const size_t new_size = utl_decode_int32(buf);
                utl_Vector* new_vector = utl_Vector_new(field->sub.vector_def, new_size);
                if(!utl_decode_vector(new_vector, def_pool, field->sub.vector_def, buf, new_size, status))
                    return false;
                utl_Vector_appendVector(vector, new_vector);
                break;
            }
        }

        /*if(value == NULL) {
            if(status) {
                status->ok = false;
                strncpy(status->message, "Expected vector value", UTL_STATUS_MAX_MESSAGE_SIZE);
            }
            return false;
        }*/
    }

    return true;
}

bool utl_decode_field(utl_Message* message, utl_DefPool* def_pool, const utl_FieldDef* field, utl_DecodeBuf* buf, utl_Status* status) {
    if(field->flag_info && field->type != FLAGS) {
        const uint8_t flag_bit = field->flag_info & 0b11111;
        const utl_FieldDef* flags_field = message->message_def->flags_fields[(field->flag_info >> 5) - 1];
        const uint32_t flags = utl_Message_getInt32(message, flags_field);
        const bool field_present = (flags & (1 << flag_bit)) == (1 << flag_bit);
        if(field->type == BIT_BOOL)
            utl_Message_setBool(message, field, field_present);
        if(!field_present)
            return true;
    }

    switch (field->type) {
        case FLAGS:
        case INT32: {
            if(!check_not_eof(buf, status, 4))
                return false;
            utl_Message_setInt32(message, field, utl_decode_int32(buf));
            break;
        }
        case INT64: {
            if(!check_not_eof(buf, status, 8))
                return false;
            utl_Message_setInt64(message, field, utl_decode_int64(buf));
            break;
        }
        case INT128: {
            if(!check_not_eof(buf, status, 16))
                return false;
            utl_Int128 tmp;
            utl_decode_int128(tmp.value, buf);
            utl_Message_setInt128(message, field, tmp);
            break;
        }
        case INT256: {
            if(!check_not_eof(buf, status, 32))
                return false;
            utl_Int256 tmp;
            utl_decode_int256(tmp.value, buf);
            utl_Message_setInt256(message, field, tmp);
            break;
        }
        case DOUBLE: {
            if(!check_not_eof(buf, status, 8))
                return false;
            utl_Message_setDouble(message, field, utl_decode_double(buf));
            break;
        }
        case FULL_BOOL: {
            if(!check_not_eof(buf, status, 4))
                return false;
            utl_Message_setBool(message, field, utl_decode_bool(buf));
            break;
        }
        case BIT_BOOL: {
            break;
        }
        case BYTES: {
            const utl_StringView bytes = utl_decode_bytes(buf, &message->arena, status);
            if(!status)
                return false;
            utl_Message_setBytes(message, field, bytes);
            break;
        }
        case STRING: {
            const utl_StringView string = utl_decode_bytes(buf, &message->arena, status);
            if(!status)
                return false;
            utl_Message_setString(message, field, string);
            break;
        }
        case TLOBJECT: {
            const uint32_t tl_id = utl_decode_int32(buf);
            utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
            if (!new_def) {
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Unknown object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                return false;
            }

            const utl_TypeDef* type = field->sub.type_def;
            if (type && new_def->type != type) {
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Invalid object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                return false;
            }

            utl_Message* new_message = utl_Message_new(new_def);
            utl_Message_setMessage(message, field, new_message);
            buf->pos += utl_decode(new_message, def_pool, buf->data + buf->pos, buf->size - buf->pos, status);
            if(!status->ok)
                return false;
            break;
        }
        case VECTOR: {
            if(utl_decode_int32(buf) != VECTOR_CONSTR) {
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Expected vector id", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                return false;
            }
            const size_t size = utl_decode_int32(buf);
            utl_Vector* vector = utl_Vector_new(field->sub.vector_def, size);
            utl_Message_setVector(message, field, vector);
            if(!utl_decode_vector(vector, def_pool, field->sub.vector_def, buf, size, status))
                return false;
            break;
        }
    }

    return true;
}

size_t utl_decode(utl_Message* out_message, utl_DefPool* def_pool, char* buf, const size_t size, utl_Status* status) {
    status->ok = true;

    utl_DecodeBuf buffer = {
        .data = buf,
        .pos = 0,
        .size = size,
    };

    const utl_FieldDef* fields = out_message->message_def->fields;
    for(int i = 0; i < out_message->message_def->fields_num; i++) {
        utl_FieldDef field = fields[i];
        if(!utl_decode_field(out_message, def_pool, &field, &buffer, status))
            return 0;
    }

    return buffer.pos;
}