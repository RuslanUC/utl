#pragma once
#include <stdbool.h>
#include "message_def.h"
#include "vector.h"

typedef struct utl_Message {
    utl_Arena arena;
    utl_MessageDef* message_def;
    void** table;
} utl_Message;

utl_Message* utl_Message_new(utl_MessageDef* message_def);
void utl_Message_free(utl_Message* message);

bool utl_Message_hasField(const utl_Message* message, const utl_FieldDef* field);
void utl_Message_clearField(const utl_Message* message, const utl_FieldDef* field);

bool utl_Message_equals(const utl_Message* a, const utl_Message* b);

void utl_Message_setInt32(utl_Message* message, const utl_FieldDef* field, int32_t value);
void utl_Message_setInt64(utl_Message* message, const utl_FieldDef* field, int64_t value);
void utl_Message_setInt128(utl_Message* message, const utl_FieldDef* field, char value[16]);
void utl_Message_setInt256(utl_Message* message, const utl_FieldDef* field, char value[32]);
void utl_Message_setDouble(utl_Message* message, const utl_FieldDef* field, double value);
void utl_Message_setBool(utl_Message* message, const utl_FieldDef* field, bool value);
void utl_Message_setBytes(utl_Message* message, const utl_FieldDef* field, utl_StringView value);
void utl_Message_setString(utl_Message* message, const utl_FieldDef* field, utl_StringView value);
void utl_Message_setMessage(const utl_Message* message, const utl_FieldDef* field, utl_Message* value);
void utl_Message_setVector(const utl_Message* message, const utl_FieldDef* field, utl_Vector* value);

int32_t utl_Message_getInt32(const utl_Message* message, const utl_FieldDef* field);
int64_t utl_Message_getInt64(const utl_Message* message, const utl_FieldDef* field);
char* utl_Message_getInt128(const utl_Message* message, const utl_FieldDef* field);
char* utl_Message_getInt256(const utl_Message* message, const utl_FieldDef* field);
double utl_Message_getDouble(const utl_Message* message, const utl_FieldDef* field);
bool utl_Message_getBool(const utl_Message* message, const utl_FieldDef* field);
utl_StringView utl_Message_getBytes(const utl_Message* message, const utl_FieldDef* field);
utl_StringView utl_Message_getString(const utl_Message* message, const utl_FieldDef* field);
utl_Message* utl_Message_getMessage(const utl_Message* message, const utl_FieldDef* field);
utl_Vector* utl_Message_getVector(const utl_Message* message, const utl_FieldDef* field);
