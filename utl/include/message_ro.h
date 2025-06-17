#pragma once
#include <stdbool.h>
#include <stdio.h>
#include "def_pool.h"
#include "message_def.h"
#include "vector_ro.h"
#include "builtins.h"

typedef struct utl_RoMessage {
    utl_MessageDef* message_def;
    utl_DefPool* def_pool;
    uint8_t* data;
    size_t size;
    ssize_t* field_positions;
} utl_RoMessage;

utl_RoMessage* utl_RoMessage_new(utl_MessageDef* message_def, utl_DefPool* def_pool, uint8_t* data, const size_t size);
void utl_RoMessage_free(utl_RoMessage* message);

bool utl_RoMessage_hasField(const utl_RoMessage* message, const utl_FieldDef* field);

bool utl_RoMessage_equals(const utl_RoMessage* a, const utl_RoMessage* b);

int32_t utl_RoMessage_getInt32(const utl_RoMessage* message, const utl_FieldDef* field);
int64_t utl_RoMessage_getInt64(const utl_RoMessage* message, const utl_FieldDef* field);
utl_Int128 utl_RoMessage_getInt128(const utl_RoMessage* message, const utl_FieldDef* field);
utl_Int256 utl_RoMessage_getInt256(const utl_RoMessage* message, const utl_FieldDef* field);
double utl_RoMessage_getDouble(const utl_RoMessage* message, const utl_FieldDef* field);
bool utl_RoMessage_getBool(const utl_RoMessage* message, const utl_FieldDef* field);
utl_StringView utl_RoMessage_getBytes(const utl_RoMessage* message, const utl_FieldDef* field);
utl_StringView utl_RoMessage_getString(const utl_RoMessage* message, const utl_FieldDef* field);
utl_RoMessage* utl_RoMessage_getMessage(const utl_RoMessage* message, const utl_FieldDef* field);
utl_RoVector* utl_RoMessage_getVector(const utl_RoMessage* message, const utl_FieldDef* field);
