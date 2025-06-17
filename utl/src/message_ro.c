#include "message_ro.h"

#include <stdio.h>
#include <stdlib.h>

#include "string.h"
#include "vector.h"
#include "builtins.h"
#include "string_pool.h"
#include "def_pool.h"
#include "decoder.h"
#include "encoder.h"

static bool utl_RoMessage_get_positions_message(utl_MessageDef* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, ssize_t* positions);
static bool utl_RoMessage_get_positions_vector(utl_MessageDefVector* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, size_t elements_count, ssize_t* positions);

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

    return false;
}

static bool skip_tlobject(utl_DecodeBuf* buffer, utl_DefPool* def_pool, utl_TypeDef* type) {
    if (!utl_DecodeBuf_read_with_oef_check(buffer, 4))
        return false;
    buffer->pos -= 4;
    const uint32_t tl_id = utl_decode_int32(buffer);
    utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
    if (!new_def)
        return false;
    if (type && new_def->type != type)
        return false;
    if(!utl_RoMessage_get_positions_message(new_def, def_pool, buffer, NULL))
        return false;

    return true;
}

static bool skip_vector(utl_DecodeBuf* buffer, utl_DefPool* def_pool, utl_MessageDefVector* vector_def) {
    if (!utl_DecodeBuf_read_with_oef_check(buffer, 8))
        return false;
    buffer->pos -= 8;
    if (utl_decode_int32(buffer) != VECTOR_CONSTR)
        return false;
    const size_t size = utl_decode_int32(buffer);
    if (!utl_RoMessage_get_positions_vector(vector_def, def_pool, buffer, size, NULL))
        return false;

    return true;
}

static bool utl_RoMessage_get_positions_vector(utl_MessageDefVector* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, size_t elements_count, ssize_t* positions) {
    const size_t buffer_start = buffer->pos;

    for(size_t i = 0; i < elements_count; i++) {
        if(positions)
            positions[i] = buffer->pos - buffer_start;

        switch (def->type) {
            case FLAGS:
            case BIT_BOOL: {
                break;
            }

            case INT32:
            case FULL_BOOL: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 4))
                    return false;
                break;
            }
            case INT64:
            case DOUBLE: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 8))
                    return false;
                break;
            }
            case INT128: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 16))
                    return false;
                break;
            }
            case INT256: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 32))
                    return false;
                break;
            }
            case BYTES:
            case STRING: {
                if(!skip_bytes(buffer))
                    return false;
                break;
            }
            case TLOBJECT: {
                if(!skip_tlobject(buffer, def_pool, def->sub.type_def))
                    return false;
                break;
            }
            case VECTOR: {
                if(!skip_vector(buffer, def_pool, def->sub.vector_def))
                    return false;
                break;
            }
        }
    }

    return true;
}

static bool utl_RoMessage_get_positions_message(utl_MessageDef* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, ssize_t* positions) {
    const size_t buffer_start = buffer->pos;
    uint32_t flags_fields[4] = {0};
    size_t flags_num = 0;

    for (int i = 0; i < def->fields_num; i++) {
        const utl_FieldDef field = def->fields[i];

        if (field.flag_info && field.type != FLAGS) {
            const uint8_t flag_bit = field.flag_info & 0b11111;
            const uint32_t flags = flags_fields[(field.flag_info >> 5) - 1];
            const bool field_present = (flags & (1 << flag_bit)) == (1 << flag_bit);
            if (field.type == BIT_BOOL && positions != NULL)
                positions[i] = -1;
            if (!field_present) {
                if (positions != NULL)
                    positions[i] = -1;
                continue;
            }
        }

        if (positions != NULL)
            positions[i] = buffer->pos - buffer_start;

        switch (field.type) {
            case FLAGS:
            case INT32:
            case FULL_BOOL: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 4))
                    return false;
                if (field.type == FLAGS) {
                    buffer->pos -= 4;
                    flags_fields[flags_num++] = utl_decode_int32(buffer);
                }
                break;
            }
            case INT64:
            case DOUBLE: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 8))
                    return false;
                break;
            }
            case INT128: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 16))
                    return false;
                break;
            }
            case INT256: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 32))
                    return false;
                break;
            }
            case BIT_BOOL: {
                break;
            }
            case BYTES:
            case STRING: {
                if(!skip_bytes(buffer))
                    return false;
                break;
            }
            case TLOBJECT: {
                if(!skip_tlobject(buffer, def_pool, field.sub.type_def))
                    return false;
                break;
            }
            case VECTOR: {
                if(!skip_vector(buffer, def_pool, field.sub.vector_def))
                    return false;
                break;
            }
        }
    }

    return true;
}

utl_RoMessage* utl_RoMessage_new(utl_MessageDef* message_def, utl_DefPool* def_pool, uint8_t* data, const size_t size) {
    utl_RoMessage* message = malloc(sizeof(utl_RoMessage) + sizeof(ssize_t) * message_def->fields_num);
    message->message_def = message_def;
    message->def_pool = def_pool;
    message->data = data;
    message->field_positions = (ssize_t*)(message + 1);

    utl_DecodeBuf buffer = {
        .data = data,
        .pos = 0,
        .size = size,
    };

    if(!utl_RoMessage_get_positions_message(message_def, def_pool, &buffer, message->field_positions)) {
        free(message);
        return false;
    }

    return message;
}

void utl_RoMessage_free(utl_RoMessage* message) {
    free(message);
}

bool utl_RoMessage_hasField(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->flag_info == 0)
        return true;

    const utl_FieldDef* flags_field = message->message_def->flags_fields[(field->flag_info >> 5) - 1];
    const uint32_t flag = *(uint32_t*)(message->data + message->field_positions[flags_field->num]);
    const uint32_t flag_bit = 1 << (field->flag_info & 0b11111);

    return (flag & flag_bit) == flag_bit;
}

bool utl_RoMessage_equals(const utl_RoMessage* a, const utl_RoMessage* b) {
    if (a == b) {
        return true;
    }
    if (a->message_def != b->message_def) {
        return false;
    }

    for (int i = 0; i < a->message_def->fields_num; i++) {
        utl_FieldDef field = a->message_def->fields[i];
        if (field.flag_info != 0 && field.type != FLAGS) {
            const bool has_a = utl_RoMessage_hasField(a, &field);
            const bool has_b = utl_RoMessage_hasField(a, &field);

            if(has_a ^ has_b)
                return false;
        }

        switch (field.type) {
            case FLAGS:
            case INT32:
            case INT64:
            case INT128:
            case INT256:
            case DOUBLE:
            case FULL_BOOL: {
                const size_t next_offset = (i == (a->message_def->fields_num - 1)) ? a->message_def->size : a->message_def->fields[i + 1].offset;
                const size_t item_size = next_offset - field.offset;
                const void* value_a = a->data + field.offset;
                const void* value_b = b->data + field.offset;

                if (memcmp(value_a, value_b, item_size))
                    return false;
                break;
            }
            case BIT_BOOL: {
                break;
            }
            case BYTES:
            case STRING: {
                if (!utl_StringView_equals(*(utl_StringView*)(a->data + field.offset), *(utl_StringView*)(b->data + field.offset)))
                    return false;
                break;
            }
            case TLOBJECT: {
                if (!utl_RoMessage_equals(utl_RoMessage_getMessage(a, &field), utl_RoMessage_getMessage(b, &field)))
                    return false;
                break;
            }
            case VECTOR: {
                if (!utl_Vector_equals(utl_RoMessage_getVector(a, &field), utl_RoMessage_getVector(b, &field)))
                    return false;
                break;
            }
        }
    }

    return true;
}

int32_t utl_RoMessage_getInt32(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != INT32 && field->type != FLAGS)
        return 0;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    return *(int32_t*)(message->data + pos);
}

int64_t utl_RoMessage_getInt64(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != INT64)
        return 0;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    return *(int64_t*)(message->data + pos);
}

utl_Int128 utl_RoMessage_getInt128(const utl_RoMessage* message, const utl_FieldDef* field) {
    utl_Int128 result = {0};
    if (field->type != INT128)
        return result;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return result;

    memcpy(result.value, message->data + pos, 16);
    return result;
}

utl_Int256 utl_RoMessage_getInt256(const utl_RoMessage* message, const utl_FieldDef* field) {
    utl_Int256 result = {0};
    if (field->type != INT256)
        return result;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return result;

    memcpy(result.value, message->data + pos, 32);
    return result;
}

double utl_RoMessage_getDouble(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != DOUBLE)
        return 0;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    return *(double*)(message->data + pos);
}

bool utl_RoMessage_getBool(const utl_RoMessage* message, const utl_FieldDef* field) {
    if(field->type == FULL_BOOL) {
        const ssize_t pos = message->field_positions[field->num];
        if(pos < 0)
            return 0;
        return *(bool*)(message->data + pos);
    } else if (field->type == BIT_BOOL) {
        return utl_RoMessage_hasField(message, field);
    }

    return false;
}

utl_StringView utl_RoMessage_getBytes_internal(const utl_RoMessage* message, const utl_FieldDef* field, const utl_FieldType check_type) {
    const utl_StringView empty = {.size = 0, .data = NULL};

    if (field->type != check_type)
        return empty;

    ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return empty;

    uint32_t count = message->data[pos++];
    if (count >= 254) {
        count = message->data[pos + 0] + (message->data[pos + 1] << 8) + (message->data[pos + 2] << 16);
        pos += 3;
    }

    return (utl_StringView){
        .data = (char*)(message->data + pos),
        .size = count,
    };
}

utl_StringView utl_RoMessage_getBytes(const utl_RoMessage* message, const utl_FieldDef* field) {
    return utl_RoMessage_getBytes_internal(message, field, BYTES);
}

utl_StringView utl_RoMessage_getString(const utl_RoMessage* message, const utl_FieldDef* field) {
    return utl_RoMessage_getBytes_internal(message, field, STRING);
}

utl_RoMessage* utl_RoMessage_getMessage(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != TLOBJECT)
        return 0;

    ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    size_t size;
    if(field->num == message->message_def->fields_num - 1) {
        size = message->size - pos;
    } else {
        size = message->field_positions[field->num + 1] - pos;
    }

    const uint32_t tl_id = *(uint32_t*)(message->data + pos);
    pos += 4;

    utl_MessageDef* new_def = utl_DefPool_getMessage(message->def_pool, tl_id);
    if (!new_def)
        return NULL;

    return utl_RoMessage_new(new_def, message->def_pool, message->data + pos, size);
}

utl_Vector* utl_RoMessage_getVector(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != VECTOR) {
        return 0;
    }

    // TODO: read RoVector
    return NULL;
}
