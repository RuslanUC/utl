#include <string.h>

#include "decoder.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "encoder.h"
#include "builtins.h"
#include "string_pool.h"
#include "constants.h"

uint8_t* utl_DecodeBuf_read(utl_DecodeBuf* buf, const size_t n) {
    if(buf->size - buf->pos < n)
        return NULL;
    uint8_t* ptr = buf->data + buf->pos;
    buf->pos += n;
    return ptr;
}

static bool check_not_eof(const utl_DecodeBuf* buf, utl_Status* status, const size_t need_bytes) {
    if(buf->size - buf->pos >= need_bytes)
        return true;
    if(status) {
        status->ok = false;
        strncpy(status->message, "Unexpected EOF", UTL_STATUS_MAX_MESSAGE_SIZE);
    }
    return false;
}

uint8_t* utl_DecodeBuf_read_with_oef_check(utl_DecodeBuf* buf, const size_t n) {
    if(!check_not_eof(buf, NULL, n))
        return NULL;
    return utl_DecodeBuf_read(buf, n);
}

void utl_decode_intX(char* value, utl_DecodeBuf* buffer, const uint8_t bytes_size) {
    const uint8_t* buf = utl_DecodeBuf_read(buffer, bytes_size);
    memcpy(value, buf, bytes_size);
}

int32_t utl_decode_int32(utl_DecodeBuf* buffer) {
    int32_t result;
    utl_decode_intX((char*)&result, buffer, 4);
    return result;
}

uint32_t utl_decode_uint32(utl_DecodeBuf* buffer) {
    uint32_t result;
    utl_decode_intX((char*)&result, buffer, 4);
    return result;
}

int64_t utl_decode_int64(utl_DecodeBuf* buffer) {
    int64_t result;
    utl_decode_intX((char*)&result, buffer, 8);
    return result;
}

void utl_decode_int128(char* out, utl_DecodeBuf* buffer) {
    const uint8_t* buf = utl_DecodeBuf_read(buffer, 16);
    memcpy(out, buf, 16);
}

void utl_decode_int256(char* out, utl_DecodeBuf* buffer) {
    const uint8_t* buf = utl_DecodeBuf_read(buffer, 32);
    memcpy(out, buf, 32);
}

double utl_decode_double(utl_DecodeBuf* buffer) {
    int64_t tmp = utl_decode_int64(buffer);
    return *(double*)&tmp;
}

bool utl_decode_bool(utl_DecodeBuf* buffer) {
    const uint8_t* buf = utl_DecodeBuf_read(buffer, 4);
    return !memcmp(buf, BOOL_TRUE, 4);
}

utl_StringView emptyStringView = {
    .size = 0,
    .data = NULL,
};

utl_StringView utl_decode_bytes(utl_DecodeBuf* buffer, utl_Status* status) {
    if(!check_not_eof(buffer, status, 1)) {
        return emptyStringView;
    }
    const uint8_t* buf = utl_DecodeBuf_read(buffer, 1);
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

    const utl_StringView result = {
        .size = count,
        .data = (char*)utl_DecodeBuf_read(buffer, count),
    };

    const uint32_t padding = (count + offset) % 4;
    if(padding) {
        buffer->pos += 4 - padding;
    }

    return result;
}

bool utl_decode_vector(utl_Vector* vector, utl_DefPool* def_pool, const utl_MessageDefVector* field, utl_DecodeBuf* buf, const int32_t size, utl_Status* status) {
    if(field->type < STATIC_FIELDS_END) {
        const uint8_t* src = utl_DecodeBuf_read(buf, size * field->element_size);
        if(src == NULL) {
            if(status) {
                status->ok = false;
                strncpy(status->message, "Unexpected end of buffer", UTL_STATUS_MAX_MESSAGE_SIZE);
            }
            return 0;
        }
        memcpy(vector->data, src, size * field->element_size);
        vector->size = size;
        return true;
    }

    for(int32_t i = 0; i < size; i++) {
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
                utl_Status tmp_status = { .ok = true };
                const utl_StringView bytes = utl_decode_bytes(buf, &tmp_status);
                if(!tmp_status.ok) {
                    if (status)
                        *status = tmp_status;
                    return false;
                }
                field->type == BYTES ? utl_Vector_appendBytes(vector, bytes) : utl_Vector_appendString(vector, bytes);
                break;
            }
            case TLOBJECT: {
                const uint32_t tl_id = utl_decode_uint32(buf);
                utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
                if (!new_def) {
                    if(status) {
                        status->ok = false;
                        strncpy(status->message, "Unknown object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                    }
                    return false;
                }

#if UTL_STRICT_TYPES
                const utl_TypeDef* type = field->sub.type_def;
                if (type && new_def->type != type) {
                    if(status) {
                        status->ok = false;
                        strncpy(status->message, "Invalid object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                    }
                    return false;
                }
#endif

                utl_Message* new_message;
                if(vector->arena == NULL)
                    new_message = utl_Message_new(new_def);
                else
                    new_message = utl_Message_new_single_alloc(new_def, vector->arena);
                utl_Status tmp_status = { .ok = true };
                buf->pos += utl_decode(new_message, def_pool, buf->data + buf->pos, buf->size - buf->pos, &tmp_status);
                if(!tmp_status.ok) {
                    if (status)
                        *status = tmp_status;
                    return false;
                }
                utl_Vector_appendMessage(vector, new_message);
                break;
            }
            case VECTOR: {
                if (utl_decode_uint32(buf) != VECTOR_CONSTR) {
                    if(status) {
                        status->ok = false;
                        strncpy(status->message, "Expected vector id", UTL_STATUS_MAX_MESSAGE_SIZE);
                    }
                    return false;
                }
                const int32_t new_size = utl_decode_int32(buf);
                utl_Vector* new_vector;
                if(vector->arena == NULL)
                    new_vector = utl_Vector_new(field->sub.vector_def, size);
                else
                    new_vector = utl_Vector_new_single_alloc(field->sub.vector_def, size, vector->arena);
                if(!utl_decode_vector(new_vector, def_pool, field->sub.vector_def, buf, new_size, status))
                    return false;
                utl_Vector_appendVector(vector, new_vector);
                break;
            }
            case STATIC_FIELDS_END:
            case ALL_FIELDS_END: {
                if(status) {
                    status->ok = false;
                    snprintf(status->message, UTL_STATUS_MAX_MESSAGE_SIZE, "Internal error: tried to serialize field type %d", field->type);
                }
                return false;
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

bool utl_decode_field(const utl_Message* message, utl_DefPool* def_pool, const utl_FieldDef* field, utl_DecodeBuf* buf, utl_Status* status) {
    if(field->flag_num && field->type != FLAGS) {
        const uint8_t flag_bit = field->flag_info;
        const utl_FieldDef* flags_field = message->message_def->flags_fields[field->flag_num - 1];
        const uint32_t flags = utl_Message_getInt32(message, flags_field);
        const bool field_present = (flags & (1 << flag_bit)) != 0;
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
        case BYTES:
        case STRING: {
            utl_Status tmp_status = { .ok = true };
            const utl_StringView string = utl_decode_bytes(buf, &tmp_status);
            if(!tmp_status.ok) {
                if (status)
                    *status = tmp_status;
                return false;
            }
            if(field->type == BYTES)
                utl_Message_setBytes(message, field, string);
            else
                utl_Message_setString(message, field, string);
            break;
        }
        case TLOBJECT: {
            const uint32_t tl_id = utl_decode_uint32(buf);
            utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
            if (!new_def) {
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Unknown object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                return false;
            }

#if UTL_STRICT_TYPES
            const utl_TypeDef* type = field->sub.type_def;
            if (type && new_def->type != type) {
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Invalid object id", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                return false;
            }
#endif

            utl_Message* new_message;
            if(message->arena == NULL)
                new_message = utl_Message_new(new_def);
            else
                new_message = utl_Message_new_single_alloc(new_def, message->arena);
            utl_Message_setMessage(message, field, new_message);
            utl_Status tmp_status = { .ok = true };
            buf->pos += utl_decode(new_message, def_pool, buf->data + buf->pos, buf->size - buf->pos, &tmp_status);
            if(!tmp_status.ok) {
                if (status)
                    *status = tmp_status;
                return false;
            }
            break;
        }
        case VECTOR: {
            if(utl_decode_uint32(buf) != VECTOR_CONSTR) {
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Expected vector id", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                return false;
            }
            const int32_t size = utl_decode_int32(buf);
            utl_Vector* vector;
            if(message->arena == NULL)
                vector = utl_Vector_new(field->sub.vector_def, size);
            else
                vector = utl_Vector_new_single_alloc(field->sub.vector_def, size, message->arena);
            utl_Message_setVector(message, field, vector);
            if(!utl_decode_vector(vector, def_pool, field->sub.vector_def, buf, size, status))
                return false;
            break;
        }
        case STATIC_FIELDS_END:
        case ALL_FIELDS_END: {
            if(status) {
                status->ok = false;
                snprintf(status->message, UTL_STATUS_MAX_MESSAGE_SIZE, "Internal error: tried to serialize field type %d", field->type);
            }
            return false;
        }
    }

    return true;
}

ssize_t utl_decode(utl_Message* out_message, utl_DefPool* def_pool, uint8_t* buf, const size_t size, utl_Status* status) {
    if (status)
        status->ok = true;

    utl_DecodeBuf buffer = {
        .data = buf,
        .pos = 0,
        .size = size,
    };

    const utl_MessageDef def = *out_message->message_def;

    if(def.fully_static) {
        const uint8_t* src = utl_DecodeBuf_read(&buffer, def.size);
        if(src == NULL) {
            if(status) {
                status->ok = false;
                strncpy(status->message, "Unexpected end of buffer", UTL_STATUS_MAX_MESSAGE_SIZE);
            }
            return -1;
        }
        memcpy(out_message->data, src, def.size);
        return def.size;
    }

    const utl_FieldDef* fields = def.fields;
    for(int i = 0; i < def.fields_num; i++) {
        utl_FieldDef field = fields[i];
        if(!utl_decode_field(out_message, def_pool, &field, &buffer, status))
            return -1;
    }

    return (ssize_t)buffer.pos;
}

ssize_t utl_decode_singlealloc(utl_Message** out, utl_MessageDef* def, utl_DefPool* def_pool, uint8_t* buf, const size_t size, utl_Status* status) {
    utl_DecodeBuf buffer = {
        .data = buf,
        .pos = 0,
        .size = size,
    };

    const ssize_t salloc_size = utl_Message_measure(def, def_pool, &buffer);
    if(salloc_size < (ssize_t)sizeof(utl_Message)) {
        return -1;
    }

    utl_StaticRefCntArena* salloc = utl_StaticRefCntArena_new(salloc_size);

    *out = utl_Message_new_single_alloc(def, salloc);
    return utl_decode(*out, def_pool, buf, size, status);
}

static bool skip_bytes(utl_DecodeBuf* buffer) {
    const uint8_t* buf = utl_DecodeBuf_read_with_oef_check(buffer, 1);
    if (!buf)
        return false;

    uint32_t count = (uint8_t)buf[0];
    uint8_t offset = 1;
    if (count >= 254) {
        buf = utl_DecodeBuf_read_with_oef_check(buffer, 3);
        if (!buf)
            return false;
        count = (uint8_t)buf[0] + ((uint8_t)buf[1] << 8) + ((uint8_t)buf[2] << 16);
        offset = 0;
    }

    const uint32_t padding = (count + offset) % 4;
    if (!utl_DecodeBuf_read_with_oef_check(buffer, count))
        return false;
    if (padding && !utl_DecodeBuf_read_with_oef_check(buffer, 4 - padding))
        return false;

    return true;
}

ssize_t utl_Message_measure(utl_MessageDef* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer) {
    ssize_t need_bytes = (ssize_t)sizeof(utl_Message) + def->size;
    if(def->fully_static) {
        if(!utl_DecodeBuf_read_with_oef_check(buffer, def->size))
            return -1;
        return need_bytes;
    }

    uint32_t flags_fields[4] = {0};
    size_t flags_num = 0;

    for (uint16_t i = 0; i < def->fields_num; i++) {
        const utl_FieldDef field = def->fields[i];

        if (field.flag_num && field.type != FLAGS) {
            const uint8_t flag_bit = field.flag_info;
            const uint32_t flags = flags_fields[field.flag_num - 1];
            const bool field_present = (flags & (1 << flag_bit)) != 0;
            if (!field_present) {
                continue;
            }
        }

        switch (field.type) {
            case FLAGS:
            case INT32:
            case FULL_BOOL:
            case INT64:
            case DOUBLE:
            case INT128:
            case INT256:
            case BIT_BOOL: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, UTL_SIZES[field.type]))
                    return -1;
                if (field.type == FLAGS) {
                    buffer->pos -= 4;
                    flags_fields[flags_num++] = utl_decode_int32(buffer);
                }
                break;
            }
            case BYTES:
            case STRING: {
                if(!skip_bytes(buffer))
                    return -1;
                break;
            }
            case TLOBJECT: {
                const uint32_t* tl_id_ptr = (uint32_t*)utl_DecodeBuf_read_with_oef_check(buffer, 4);
                if(tl_id_ptr == NULL)
                    return -1;

                utl_MessageDef* obj_def = utl_DefPool_getMessage(def_pool, *tl_id_ptr);
                if(obj_def == NULL)
                    return -1;

                const ssize_t obj_size = utl_Message_measure(obj_def, def_pool, buffer);
                if(obj_size < (ssize_t)sizeof(utl_Message))
                    return -1;

                need_bytes += obj_size;
                break;
            }
            case VECTOR: {
                const uint32_t* tl_id_ptr = (uint32_t*)utl_DecodeBuf_read_with_oef_check(buffer, 4);
                if(tl_id_ptr == NULL)
                    return -1;

                if(*tl_id_ptr != VECTOR_CONSTR)
                    return -1;

                const int32_t* length_ptr = (int32_t*)utl_DecodeBuf_read_with_oef_check(buffer, 4);
                if(length_ptr == NULL)
                    return -1;

                const ssize_t obj_size = utl_Vector_measure(field.sub.vector_def, *length_ptr, def_pool, buffer);
                if(obj_size < (ssize_t)sizeof(utl_Vector))
                    return -1;

                need_bytes += obj_size;
                break;
            }

            // Must be unreachable
            case STATIC_FIELDS_END:
            case ALL_FIELDS_END: return -1;
        }
    }

    return need_bytes;
}

ssize_t utl_Vector_measure(utl_MessageDefVector* vector_def, const int32_t length, utl_DefPool* def_pool, utl_DecodeBuf* buffer) {
    ssize_t need_bytes = (ssize_t)sizeof(utl_Vector) + vector_def->element_size * length;
    if(vector_def->type < STATIC_FIELDS_END) {
        if(!utl_DecodeBuf_read_with_oef_check(buffer, vector_def->element_size * length))
            return -1;
        return need_bytes;
    }

    for(int32_t i = 0; i < length; i++) {
        switch (vector_def->type) {
            case FLAGS:
            case BIT_BOOL:
            case INT32:
            case FULL_BOOL:
            case INT64:
            case DOUBLE:
            case INT128:
            case INT256:
                return -1;

            case BYTES:
            case STRING: {
                if(!skip_bytes(buffer))
                    return -1;
                break;
            }
            case TLOBJECT: {
                const uint32_t* tl_id_ptr = (uint32_t*)utl_DecodeBuf_read_with_oef_check(buffer, 4);
                if(tl_id_ptr == NULL)
                    return -1;

                utl_MessageDef* obj_def = utl_DefPool_getMessage(def_pool, *tl_id_ptr);
                if(obj_def == NULL)
                    return -1;

                const ssize_t obj_size = utl_Message_measure(obj_def, def_pool, buffer);
                if(obj_size < (ssize_t)sizeof(utl_Message))
                    return -1;

                need_bytes += obj_size;
                break;
            }
            case VECTOR: {
                const uint32_t* tl_id_ptr = (uint32_t*)utl_DecodeBuf_read_with_oef_check(buffer, 4);
                if(tl_id_ptr == NULL)
                    return -1;

                if(*tl_id_ptr != VECTOR_CONSTR)
                    return -1;

                const int32_t* length_ptr = (int32_t*)utl_DecodeBuf_read_with_oef_check(buffer, 4);
                if(length_ptr == NULL)
                    return -1;

                const ssize_t obj_size = utl_Vector_measure(vector_def->sub.vector_def, *length_ptr, def_pool, buffer);
                if(obj_size < (ssize_t)sizeof(utl_Vector))
                    return -1;

                need_bytes += obj_size;
                break;
            }

                // Must be unreachable
            case STATIC_FIELDS_END:
            case ALL_FIELDS_END: return -1;
        }
    }

    return need_bytes;
}

