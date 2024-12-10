#include "message.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector.h>

#include "builtins.h"

utl_Message* utl_Message_new(utl_MessageDef* message_def) {
    utl_Arena arena = utl_Arena_new(4096);
    utl_Message* message = utl_Arena_alloc(&arena, sizeof(utl_Message));
    message->message_def = message_def;
    message->table = malloc(message_def->fields_num * sizeof(void*));

    for (int i = 0; i < message_def->fields_num; i++) {
        const utl_FieldDef field = message_def->fields[i];
        if (field.flag_info != 0 && field.type != FLAGS) {
            message->table[i] = NULL;
            continue;
        }
        switch (field.type) {
            case FLAGS:
            case INT32: {
                message->table[i] = utl_Arena_alloc(&arena, sizeof(utl_Int32));
                break;
            }
            case INT64: {
                message->table[i] = utl_Arena_alloc(&arena, sizeof(utl_Int64));
                break;
            }
            case INT128: {
                message->table[i] = utl_Arena_alloc(&arena, sizeof(utl_Int128));
                break;
            }
            case INT256: {
                message->table[i] = utl_Arena_alloc(&arena, sizeof(utl_Int256));
                break;
            }
            case DOUBLE: {
                message->table[i] = utl_Arena_alloc(&arena, sizeof(utl_Double));
                break;
            }
            case FULL_BOOL:
            case BIT_BOOL: {
                message->table[i] = utl_Arena_alloc(&arena, sizeof(utl_Bool));
                break;
            }
            case BYTES:
            case STRING:
            case TLOBJECT:
            case VECTOR:{
                message->table[i] = NULL;
                break;
            }
        }
    }

    message->arena = arena;
    return message;
}

void utl_Message_free(utl_Message* message) {
    free(message->table);
    utl_Arena_free(&message->arena);
}

bool utl_Message_hasField(const utl_Message* message, const utl_FieldDef* field) {
    if (field->flag_info == 0)
        return true;

    return message->table[field->num] != 0;
}

void utl_Message_clearField(const utl_Message* message, const utl_FieldDef* field) {
    if (field->flag_info == 0)
        return;

    message->table[field->num] = 0;
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
        if (field.type == FLAGS) {
            continue;
        }
        if (field.flag_info != 0) {
            const bool has_a = utl_Message_hasField(a, &field);
            const bool has_b = utl_Message_hasField(a, &field);

            return !(has_a ^ has_b);
        }

        switch (field.type) {
            case FLAGS:
            case INT32: {
                if (utl_Message_getInt32(a, &field) != utl_Message_getInt32(b, &field))
                    return false;
                break;
            }
            case INT64: {
                if (utl_Message_getInt64(a, &field) != utl_Message_getInt64(b, &field))
                    return false;
                break;
            }
            case INT128: {
                const char* ia = utl_Message_getInt128(a, &field);
                const char* ib = utl_Message_getInt128(b, &field);
                if (memcmp(ia, ib, 16))
                    return false;
                break;
            }
            case INT256: {
                const char* ia = utl_Message_getInt256(a, &field);
                const char* ib = utl_Message_getInt256(b, &field);
                if (memcmp(ia, ib, 32))
                    return false;
                break;
            }
            case DOUBLE: {
                if (utl_Message_getDouble(a, &field) != utl_Message_getDouble(b, &field))
                    return false;
                break;
            }
            case FULL_BOOL:
            case BIT_BOOL: {
                if (utl_Message_getBool(a, &field) != utl_Message_getBool(b, &field))
                    return false;
                break;
            }
            case BYTES: {
                if (!utl_StringView_equals(utl_Message_getBytes(a, &field), utl_Message_getBytes(b, &field)))
                    return false;
                break;
            }
            case STRING: {
                if (!utl_StringView_equals(utl_Message_getString(a, &field), utl_Message_getString(b, &field)))
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

void utl_Message_setInt32(utl_Message* message, const utl_FieldDef* field, const int32_t value) {
    if (field->type != INT32 && field->type != FLAGS) {
        return;
    }

    if (message->table[field->num] == NULL) {
        message->table[field->num] = utl_Arena_alloc(&message->arena, sizeof(utl_Int32));
    }
    ((utl_Int32*)message->table[field->num])->value = value;
}

void utl_Message_setInt64(utl_Message* message, const utl_FieldDef* field, const int64_t value) {
    if (field->type != INT64) {
        return;
    }

    if (message->table[field->num] == NULL) {
        message->table[field->num] = utl_Arena_alloc(&message->arena, sizeof(utl_Int64));
    }
    ((utl_Int64*)message->table[field->num])->value = value;
}

void utl_Message_setInt128(utl_Message* message, const utl_FieldDef* field, char value[16]) {
    if (field->type != INT128) {
        return;
    }

    if (message->table[field->num] == NULL) {
        message->table[field->num] = utl_Arena_alloc(&message->arena, sizeof(utl_Int128));
    }
    memcpy(&((utl_Int128*)message->table[field->num])->value, value, 16);
}

void utl_Message_setInt256(utl_Message* message, const utl_FieldDef* field, char value[32]) {
    if (field->type != INT256) {
        return;
    }

    if (message->table[field->num] == NULL) {
        message->table[field->num] = utl_Arena_alloc(&message->arena, sizeof(utl_Int256));
    }
    memcpy(&((utl_Int256*)message->table[field->num])->value, value, 32);
}

void utl_Message_setDouble(utl_Message* message, const utl_FieldDef* field, const double value) {
    if (field->type != DOUBLE) {
        return;
    }

    if (message->table[field->num] == NULL) {
        message->table[field->num] = utl_Arena_alloc(&message->arena, sizeof(utl_Double));
    }
    ((utl_Double*)message->table[field->num])->value = value;
}

void utl_Message_setBool(utl_Message* message, const utl_FieldDef* field, const bool value) {
    if (field->type != FULL_BOOL && field->type != BIT_BOOL) {
        return;
    }

    if (message->table[field->num] == NULL) {
        message->table[field->num] = utl_Arena_alloc(&message->arena, sizeof(utl_Bool));
    }
    ((utl_Bool*)message->table[field->num])->value = value;
}

void utl_Message_setBytes_internal(utl_Message* message, const utl_FieldDef* field, const utl_StringView value, const bool check_type) {
    if (check_type && field->type != BYTES) {
        return;
    }

    if (message->table[field->num] == NULL) {
        message->table[field->num] = utl_Arena_alloc(&message->arena, sizeof(utl_Bytes));
        memset(message->table[field->num], 0, sizeof(utl_Bytes));
    }

    utl_Bytes* bytes = message->table[field->num];
    if (value.size > bytes->max_size) {
        bytes->value.data = utl_Arena_alloc(&message->arena, value.size);
        bytes->max_size = value.size;
    }

    bytes->value.size = value.size;
    memcpy(bytes->value.data, value.data, value.size);
}

void utl_Message_setBytes(utl_Message* message, const utl_FieldDef* field, const utl_StringView value) {
    utl_Message_setBytes_internal(message, field, value, true);
}

void utl_Message_setString(utl_Message* message, const utl_FieldDef* field, const utl_StringView value) {
    utl_Message_setBytes_internal(message, field, value, false);
}

void utl_Message_setMessage(const utl_Message* message, const utl_FieldDef* field, utl_Message* value) {
    if (field->type != TLOBJECT) {
        return;
    }

    message->table[field->num] = value;
}

void utl_Message_setVector(const utl_Message* message, const utl_FieldDef* field, utl_Vector* value) {
    if (field->type != VECTOR) {
        return;
    }

    message->table[field->num] = value;
}

int32_t utl_Message_getInt32(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != INT32 && field->type != FLAGS) {
        return 0;
    }

    if (message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int32*)message->table[field->num])->value;
}

int64_t utl_Message_getInt64(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != INT64) {
        return 0;
    }

    if (message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int64*)message->table[field->num])->value;
}

char* utl_Message_getInt128(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != INT128) {
        return 0;
    }

    if (message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int128*)message->table[field->num])->value;
}

char* utl_Message_getInt256(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != INT256) {
        return 0;
    }

    if (message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int256*)message->table[field->num])->value;
}

double utl_Message_getDouble(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != DOUBLE) {
        return 0;
    }

    if (message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Double*)message->table[field->num])->value;
}

bool utl_Message_getBool(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != FULL_BOOL && field->type != BIT_BOOL) {
        return 0;
    }

    if (message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Bool*)message->table[field->num])->value;
}

utl_StringView utl_Message_getBytes_internal(const utl_Message* message, const utl_FieldDef* field, const utl_FieldType check_type) {
    const utl_StringView empty = {.size = 0, .data = NULL};

    if (field->type != check_type) {
        return empty;
    }

    if (message->table[field->num] == NULL) {
        return empty;
    }

    return ((utl_Bytes*)message->table[field->num])->value;
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

    return message->table[field->num];
}

utl_Vector* utl_Message_getVector(const utl_Message* message, const utl_FieldDef* field) {
    if (field->type != VECTOR) {
        return 0;
    }

    return message->table[field->num];
}
