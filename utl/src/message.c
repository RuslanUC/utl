#include "message.h"

#include <constants.h>
#include <stdio.h>
#include <stdlib.h>

#include "string.h"
#include "vector.h"
#include "builtins.h"
#include "string_pool.h"

utl_Message* utl_Message_new(utl_MessageDef* message_def) {
    utl_Arena arena = utl_Arena_new(4096);
    utl_Message* message = utl_Arena_alloc(&arena, sizeof(utl_Message));
    message->message_def = message_def;
    message->data = malloc(message_def->size);
    memset(message->data, 0, message_def->size);
    message->arena = arena;
    return message;
}

void utl_Message_free(utl_Message* message) {
    for(int i = 0; i < message->message_def->strings_num; ++i) {
        const utl_FieldDef* field = message->message_def->string_fields[i];
        const utl_StringView string = field->type == STRING ? utl_Message_getString(message, field) : utl_Message_getBytes(message, field);
        if(!string.data)
            continue;
        utl_StringPool_free(string);
    }

    free(message->data);
    utl_Arena_free(&message->arena);
}

bool utl_Message_hasField(const utl_Message* message, const utl_FieldDef* field) {
    if (field->flag_info == 0)
        return true;

    const utl_FieldDef* flags_field = message->message_def->flags_fields[(field->flag_info >> 5) - 1];
    const uint32_t flag = *(uint32_t*)(message->data + flags_field->offset);
    const uint32_t flag_bit = 1 << (field->flag_info & 0b11111);

    return (flag & flag_bit) == flag_bit;
}

void utl_Message_clearField(const utl_Message* message, const utl_FieldDef* field) {
    if (field->flag_info == 0)
        return;

    const utl_FieldDef* flags_field = message->message_def->flags_fields[(field->flag_info >> 5) - 1];
    const uint32_t flag_bit = 1 << (field->flag_info & 0b11111);
    *(uint32_t*)(message->data + flags_field->offset) &= ~flag_bit;
}

void utl_Message_setFlag(const utl_Message* message, const utl_FieldDef* field) {
    if (field->flag_info == 0)
        return;

    const utl_FieldDef* flags_field = message->message_def->flags_fields[(field->flag_info >> 5) - 1];
    const uint32_t flag_bit = 1 << (field->flag_info & 0b11111);
    *(uint32_t*)(message->data + flags_field->offset) |= flag_bit;
}

bool utl_Message_equals(const utl_Message* a, const utl_Message* b) {
    if (a == b) {
        return true;
    }
    if (a->message_def != b->message_def) {
        return false;
    }

    for (int i = 0; i < a->message_def->fields_num; i++) {
        utl_FieldDef field = a->message_def->fields[i];
        if (field.flag_info != 0 && field.type != FLAGS) {
            const bool has_a = utl_Message_hasField(a, &field);
            const bool has_b = utl_Message_hasField(a, &field);

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
                if (!utl_Message_equals(utl_Message_getMessage(a, &field), utl_Message_getMessage(b, &field)))
                    return false;
                break;
            }
            case VECTOR: {
                if (!utl_Vector_equals(utl_Message_getVector(a, &field), utl_Message_getVector(b, &field)))
                    return false;
                break;
            }
        }
    }

    return true;
}

void utl_Message_setInt32(const utl_Message* message, const utl_FieldDef* field, const int32_t value) {
    if (field->type != INT32 && field->type != FLAGS) {
        return;
    }

    utl_Message_setFlag(message, field);
    *(int32_t*)(message->data + field->offset) = value;
}

void utl_Message_setInt64(const utl_Message* message, const utl_FieldDef* field, const int64_t value) {
    if (field->type != INT64) {
        return;
    }

    utl_Message_setFlag(message, field);
    *(int64_t*)(message->data + field->offset) = value;
}

void utl_Message_setInt128(const utl_Message* message, const utl_FieldDef* field, const utl_Int128 value) {
    if (field->type != INT128) {
        return;
    }

    utl_Message_setFlag(message, field);
    memcpy(message->data + field->offset, value.value, 16);
}

void utl_Message_setInt256(const utl_Message* message, const utl_FieldDef* field, const utl_Int256 value) {
    if (field->type != INT256) {
        return;
    }

    utl_Message_setFlag(message, field);
    memcpy(message->data + field->offset, value.value, 32);
}

void utl_Message_setDouble(const utl_Message* message, const utl_FieldDef* field, const double value) {
    if (field->type != DOUBLE) {
        return;
    }

    utl_Message_setFlag(message, field);
    *(double*)(message->data + field->offset) = value;
}

void utl_Message_setBool(const utl_Message* message, const utl_FieldDef* field, const bool value) {
    if(field->type == FULL_BOOL) {
        utl_Message_setFlag(message, field);
        *(bool*)(message->data + field->offset) = value;
    } else if (field->type == BIT_BOOL) {
        if(value)
            utl_Message_setFlag(message, field);
        else
            utl_Message_clearField(message, field);
    }
}

void utl_Message_setBytes_internal(const utl_Message* message, const utl_FieldDef* field, const utl_StringView value, const utl_FieldType check_type) {
    if (field->type != check_type || value.size > UTL_MAX_STRINT_LENGTH) {
        return;
    }

    utl_StringView* bytes = message->data + field->offset;
    *bytes = utl_StringPool_realloc(*bytes, value.size);
    memcpy(bytes->data, value.data, value.size);

    utl_Message_setFlag(message, field);
}

void utl_Message_setBytes(const utl_Message* message, const utl_FieldDef* field, const utl_StringView value) {
    utl_Message_setBytes_internal(message, field, value, BYTES);
}

void utl_Message_setString(const utl_Message* message, const utl_FieldDef* field, const utl_StringView value) {
    utl_Message_setBytes_internal(message, field, value, STRING);
}

void utl_Message_setMessage(const utl_Message* message, const utl_FieldDef* field, utl_Message* value) {
    if (field->type != TLOBJECT) {
        return;
    }

    utl_Message_setFlag(message, field);
    *(utl_Message**)(message->data + field->offset) = value;
}

void utl_Message_setVector(const utl_Message* message, const utl_FieldDef* field, utl_Vector* value) {
    if (field->type != VECTOR) {
        return;
    }

    utl_Message_setFlag(message, field);
    *(utl_Vector**)(message->data + field->offset) = value;
}

int32_t utl_Message_getInt32(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != INT32 && field->type != FLAGS) {
        return 0;
    }

    return *(int32_t*)(message->data + field->offset);
}

int64_t utl_Message_getInt64(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != INT64) {
        return 0;
    }

    return *(int64_t*)(message->data + field->offset);
}

utl_Int128 utl_Message_getInt128(const utl_Message* message, const utl_FieldDef* field) {
    utl_Int128 result;
    if (field->type != INT128) {
        return result;
    }

    memcpy(result.value, message->data + field->offset, 16);
    return result;
}

utl_Int256 utl_Message_getInt256(const utl_Message* message, const utl_FieldDef* field) {
    utl_Int256 result;
    if (field->type != INT256) {
        return result;
    }

    memcpy(result.value, message->data + field->offset, 32);
    return result;
}

double utl_Message_getDouble(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != DOUBLE) {
        return 0;
    }

    return *(double*)(message->data + field->offset);
}

bool utl_Message_getBool(const utl_Message* message, const utl_FieldDef* field) {
    if(field->type == FULL_BOOL) {
        return *(bool*)(message->data + field->offset);
    } else if (field->type == BIT_BOOL) {
        return utl_Message_hasField(message, field);
    }
    return false;
}

utl_StringView utl_Message_getBytes_internal(const utl_Message* message, const utl_FieldDef* field, const utl_FieldType check_type) {
    const utl_StringView empty = {.size = 0, .data = NULL};

    if (field->type != check_type) {
        return empty;
    }

    const utl_StringView* bytes = message->data + field->offset;
    if (bytes->data == NULL) {
        return empty;
    }

    return *bytes;
}

utl_StringView utl_Message_getBytes(const utl_Message* message, const utl_FieldDef* field) {
    return utl_Message_getBytes_internal(message, field, BYTES);
}

utl_StringView utl_Message_getString(const utl_Message* message, const utl_FieldDef* field) {
    return utl_Message_getBytes_internal(message, field, STRING);
}

utl_Message* utl_Message_getMessage(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != TLOBJECT) {
        return 0;
    }

    return *(utl_Message**)(message->data + field->offset);
}

utl_Vector* utl_Message_getVector(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != VECTOR) {
        return 0;
    }

    return *(utl_Vector**)(message->data + field->offset);
}
