#include <string.h>

#include "decoder.h"
#include "encoder.h"

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
    if(count >= 254) {
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

void utl_decode_vector(utl_Vector* vector, utl_DefPool* def_pool, utl_MessageDefVector* field, utl_DecodeBuf* buf, size_t size) {
    for(size_t i = 0; i < size; i++) {
        void* value = NULL;

        switch (field->type) {
            case FLAGS:
            case INT32: {
                value = arena_alloc(&vector->arena, sizeof(utl_Int32));
                ((utl_Int32*)value)->value = utl_decode_int32(buf);
                break;
            }
            case INT64: {
                value = arena_alloc(&vector->arena, sizeof(utl_Int64));
                ((utl_Int64*)value)->value = utl_decode_int64(buf);
                break;
            }
            case INT128: {
                value = arena_alloc(&vector->arena, sizeof(utl_Int128));
                utl_decode_int128(((utl_Int128*)value)->value, buf);
                break;
            }
            case INT256: {
                value = arena_alloc(&vector->arena, sizeof(utl_Int256));
                utl_decode_int128(((utl_Int256*)value)->value, buf);
                break;
            }
            case DOUBLE: {
                value = arena_alloc(&vector->arena, sizeof(utl_Double));
                ((utl_Double*)value)->value = utl_decode_double(buf);
                break;
            }
            case FULL_BOOL: {
                value = arena_alloc(&vector->arena, sizeof(utl_Bool));
                ((utl_Bool*)value)->value = utl_decode_bool(buf);
                break;
            }
            case BIT_BOOL: {
                break;
            }
            case BYTES: {
                value = arena_alloc(&vector->arena, sizeof(utl_Bytes));
                ((utl_Bytes*)value)->value = utl_decode_bytes(buf, &vector->arena);
                ((utl_Bytes*)value)->max_size = ((utl_Bytes*)value)->value.size;
                break;
            }
            case STRING: {
                value = arena_alloc(&vector->arena, sizeof(utl_String));
                ((utl_String*)value)->value = utl_decode_bytes(buf, &vector->arena);
                ((utl_String*)value)->max_size = ((utl_String*)value)->value.size;
                break;
            }
            case TLOBJECT: {
                utl_TypeDef* type = (utl_TypeDef*)field->sub_message_def;
                // TODO: handle case when type is NULL (object type is any, "!X" or "TLObject"), maybe simply skip type check?

                uint32_t tl_id = utl_decode_int32(buf);
                utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
                if (!new_def || new_def->type != type) {
                    // TODO: fail
                }

                utl_Message* new_message = utl_Message_new(new_def);
                value = new_message;
                buf->pos += utl_decode(new_message, def_pool, buf->data + buf->pos, buf->size - buf->pos);
                break;
            }
            case VECTOR: {
                if (utl_decode_int32(buf) != VECTOR_CONSTR) {
                    // TODO: fail
                }
                size_t new_size = utl_decode_int32(buf);
                utl_Vector* new_vector = utl_Vector_new((utl_MessageDefVector*)field->sub_message_def, new_size);
                value = new_vector;
                utl_decode_vector(new_vector, def_pool, (utl_MessageDefVector*)field->sub_message_def, buf, new_size);
                break;
            }
        }

        if(value == NULL)
            continue; // TODO: fail?

        utl_Vector_append(vector, value);
    }
}

void utl_decode_field(utl_Message* message, utl_DefPool* def_pool, utl_FieldDef* field, utl_DecodeBuf* buf) {
    if(field->flag_info && field->type != FLAGS) {
        uint8_t flag_bit = field->flag_info & 0b11111;
        utl_FieldDef flags_field = message->message_def->flags_fields[(field->flag_info >> 5) - 1];
        uint32_t flags = utl_Message_getInt32(message, &flags_field);
        bool field_present = (flags & (1 << flag_bit)) == (1 << flag_bit);
        if(field->type == BIT_BOOL)
            utl_Message_setBool(message, field, field_present);
        if(!field_present)
            return;
    }

    switch (field->type) {
        case FLAGS:
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
        case FULL_BOOL: {
            utl_Message_setBool(message, field, utl_decode_bool(buf));
            break;
        }
        case BIT_BOOL: {
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
            utl_TypeDef* type = (utl_TypeDef*)field->sub_message_def;
            // TODO: handle case when type is NULL (object type is any, "!X" or "TLObject"), maybe simply skip type check?

            uint32_t tl_id = utl_decode_int32(buf);
            utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
            if(!new_def || new_def->type != type) {
                // TODO: fail
            }

            utl_Message* new_message = utl_Message_new(new_def);
            utl_Message_setMessage(message, field, new_message);
            buf->pos += utl_decode(new_message, def_pool, buf->data + buf->pos, buf->size - buf->pos);
            break;
        }
        case VECTOR: {
            if(utl_decode_int32(buf) != VECTOR_CONSTR) {
                // TODO: fail
            }
            size_t size = utl_decode_int32(buf);
            utl_Vector* vector = utl_Vector_new((utl_MessageDefVector*)field->sub_message_def, size);
            utl_Message_setVector(message, field, vector);
            utl_decode_vector(vector, def_pool, (utl_MessageDefVector*)field->sub_message_def, buf, size);
            break;
        }
    }
}

size_t utl_decode(utl_Message* message, utl_DefPool* def_pool, char* buf, size_t size) {
    utl_DecodeBuf buffer = {
        .data = buf,
        .pos = 0,
        .size = size,
    };

    const utl_FieldDef* fields = message->message_def->fields;
    for(int i = 0; i < message->message_def->fields_num; i++) {
        utl_FieldDef field = fields[i];
        utl_decode_field(message, def_pool, &field, &buffer);
    }

    return buffer.pos;
}