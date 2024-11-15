#include "message.h"

#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <vector.h>

#include "builtins.h"

utl_Message* utl_Message_new(utl_MessageDef* message_def) {
    arena_t arena = arena_new();
    utl_Message* message = arena_alloc(&arena, sizeof(utl_Message));
    message->message_def = message_def;
    message->table = arena_alloc(&arena, sizeof(message_def->fields_num) * sizeof(void*));

    for(int i = 0; i < message_def->fields_num; i++) {
        const utl_FieldDef field = message_def->fields[i];
        switch (field.type) {
            case INT32: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Int32));
                break;
            }
            case INT64: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Int64));
                break;
            }
            case INT128: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Int128));
                break;
            }
            case INT256: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Int256));
                break;
            }
            case DOUBLE: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Double));
                break;
            }
            case BOOL: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Bool));
                break;
            }
            case BYTES: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Bytes));
                break;
            }
            case STRING: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_String));
                break;
            }
            case TLOBJECT: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Message*));
                break;
            }
            case VECTOR: {
                message->table[i] = arena_alloc(&arena, sizeof(utl_Vector*));
                break;
            }
        }
    }

    message->arena = arena;
    return message;
}

void utl_Message_free(utl_Message* message) {
    arena_delete(&message->arena);
}

void utl_Message_setInt32(utl_Message* message, utl_FieldDef* field, int32_t value){
    if(field->type != INT32) {
        return;
    }

    if(message->table[field->num] == NULL) {
        message->table[field->num] = arena_alloc(&message->arena, sizeof(utl_Int32));
    }
    ((utl_Int32*)message->table[field->num])->value = value;
}

void utl_Message_setInt64(utl_Message* message, utl_FieldDef* field, int64_t value){
    if(field->type != INT64) {
        return;
    }

    if(message->table[field->num] == NULL) {
        message->table[field->num] = arena_alloc(&message->arena, sizeof(utl_Int64));
    }
    ((utl_Int64*)message->table[field->num])->value = value;
}

void utl_Message_setInt128(utl_Message* message, utl_FieldDef* field, char value[16]){
    if(field->type != INT128) {
        return;
    }

    if(message->table[field->num] == NULL) {
        message->table[field->num] = arena_alloc(&message->arena, sizeof(utl_Int128));
    }
    memcpy(&((utl_Int128*)message->table[field->num])->value, &value, 16);
}

void utl_Message_setInt256(utl_Message* message, utl_FieldDef* field, char value[32]){
    if(field->type != INT256) {
        return;
    }

    if(message->table[field->num] == NULL) {
        message->table[field->num] = arena_alloc(&message->arena, sizeof(utl_Int256));
    }
    memcpy(&((utl_Int256*)message->table[field->num])->value, &value, 32);
}

void utl_Message_setDouble(utl_Message* message, utl_FieldDef* field, double value){
    if(field->type != DOUBLE) {
        return;
    }

    if(message->table[field->num] == NULL) {
        message->table[field->num] = arena_alloc(&message->arena, sizeof(utl_Double));
    }
    ((utl_Double*)message->table[field->num])->value = value;
}

void utl_Message_setBool(utl_Message* message, utl_FieldDef* field, bool value){
    if(field->type != BOOL) {
        return;
    }

    if(message->table[field->num] == NULL) {
        message->table[field->num] = arena_alloc(&message->arena, sizeof(utl_Bool));
    }
    ((utl_Bool*)message->table[field->num ])->value = value;
}

void utl_Message_setBytes_internal(utl_Message* message, utl_FieldDef* field, utl_StringView value, bool check_type){
    if(check_type && field->type != BYTES) {
        return;
    }

    if(message->table[field->num] == NULL) {
        message->table[field->num] = arena_alloc(&message->arena, sizeof(utl_Bytes));
    }

    utl_Bytes* bytes = message->table[field->num];
    if(value.size > bytes->max_size) {
        bytes->value.data = arena_alloc(&message->arena, value.size);
        bytes->max_size = value.size;
    }

    bytes->value.size = value.size;
    memcpy(bytes->value.data, value.data, value.size);
}

void utl_Message_setBytes(utl_Message* message, utl_FieldDef* field, utl_StringView value){
    utl_Message_setBytes_internal(message, field, value, true);
}

void utl_Message_setString(utl_Message* message, utl_FieldDef* field, utl_StringView value){
    utl_Message_setBytes_internal(message, field, value, false);
}

void utl_Message_setMessage(utl_Message* message, utl_FieldDef* field, utl_Message* value){
    if(field->type != TLOBJECT) {
        return;
    }

    message->table[field->num] = value;
}

int32_t utl_Message_getInt32(utl_Message* message, utl_FieldDef* field){
    if(field->type != INT32) {
        return 0;
    }

    if(message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int32*)message->table[field->num])->value;
}

int64_t utl_Message_getInt64(utl_Message* message, utl_FieldDef* field){
    if(field->type != INT64) {
        return 0;
    }

    if(message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int64*)message->table[field->num])->value;
}

char* utl_Message_getInt128(utl_Message* message, utl_FieldDef* field){
    if(field->type != INT128) {
        return 0;
    }

    if(message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int128*)message->table[field->num])->value;
}

char* utl_Message_getInt256(utl_Message* message, utl_FieldDef* field){
    if(field->type != INT256) {
        return 0;
    }

    if(message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Int256*)message->table[field->num])->value;
}

double utl_Message_getDouble(utl_Message* message, utl_FieldDef* field){
    if(field->type != DOUBLE) {
        return 0;
    }

    if(message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Double*)message->table[field->num])->value;
}

bool utl_Message_getBool(utl_Message* message, utl_FieldDef* field){
    if(field->type != BOOL) {
        return 0;
    }

    if(message->table[field->num] == NULL) {
        return 0;
    }
    return ((utl_Bool*)message->table[field->num])->value;
}

utl_StringView utl_Message_getBytes_internal(utl_Message* message, utl_FieldDef* field, utl_FieldType check_type){
    utl_StringView empty = {.size = 0, .data = NULL};

    if(field->type != check_type) {
        return empty;
    }

    if(message->table[field->num] == NULL) {
        return empty;
    }

    return ((utl_Bytes*)message->table[field->num])->value;
}

utl_StringView utl_Message_getBytes(utl_Message* message, utl_FieldDef* field){
    return utl_Message_getBytes_internal(message, field, BYTES);
}

utl_StringView utl_Message_getString(utl_Message* message, utl_FieldDef* field){
    return utl_Message_getBytes_internal(message, field, STRING);
}

utl_Message* utl_Message_getMessage(utl_Message* message, utl_FieldDef* field){
    if(field->type != TLOBJECT) {
        return 0;
    }

    return message->table[field->num];
}

