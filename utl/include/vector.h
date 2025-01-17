#pragma once

#include <stdbool.h>

#include "message_def.h"
#include "builtins.h"

struct utl_Message;

typedef struct utl_Vector {
    utl_Arena arena;
    utl_MessageDefVector* message_def;
    size_t size;
    size_t capacity;
    void* data;
} utl_Vector;

utl_Vector* utl_Vector_new(utl_MessageDefVector* vector_def, size_t initial_size);
void utl_Vector_free(utl_Vector* vector);
size_t utl_Vector_capacity(const utl_Vector* vector);
void utl_Vector_remove(utl_Vector* vector, size_t index);
size_t utl_Vector_size(const utl_Vector* vector);
bool utl_Vector_equals(const utl_Vector* a, const utl_Vector* b);
void* utl_Vector_rawValue(const utl_Vector* vector, const size_t index);

void utl_Vector_appendInt32(utl_Vector* vector, int32_t value);
void utl_Vector_appendInt64(utl_Vector* vector, int64_t value);
void utl_Vector_appendInt128(utl_Vector* vector, utl_Int128 value);
void utl_Vector_appendInt256(utl_Vector* vector, utl_Int256 value);
void utl_Vector_appendDouble(utl_Vector* vector, double value);
void utl_Vector_appendBool(utl_Vector* vector, bool value);
void utl_Vector_appendBytes(utl_Vector* vector, utl_StringView value);
void utl_Vector_appendString(utl_Vector* vector, utl_StringView value);
void utl_Vector_appendMessage(utl_Vector* vector, struct utl_Message* value);
void utl_Vector_appendVector(utl_Vector* vector, utl_Vector* value);

void utl_Vector_setInt32(const utl_Vector* vector, const size_t index, int32_t value);
void utl_Vector_setInt64(const utl_Vector* vector, const size_t index, int64_t value);
void utl_Vector_setInt128(const utl_Vector* vector, const size_t index, utl_Int128 value);
void utl_Vector_setInt256(const utl_Vector* vector, const size_t index, utl_Int256 value);
void utl_Vector_setDouble(const utl_Vector* vector, const size_t index, double value);
void utl_Vector_setBool(const utl_Vector* vector, const size_t index, bool value);
void utl_Vector_setBytes(utl_Vector* vector, const size_t index, utl_StringView value);
void utl_Vector_setString(utl_Vector* vector, const size_t index, utl_StringView value);
void utl_Vector_setMessage(const utl_Vector* vector, const size_t index, struct utl_Message* value);
void utl_Vector_setVector(const utl_Vector* vector, const size_t index, utl_Vector* value);

int32_t utl_Vector_getInt32(const utl_Vector* vector, const size_t index);
int64_t utl_Vector_getInt64(const utl_Vector* vector, const size_t index);
utl_Int128 utl_Vector_getInt128(const utl_Vector* vector, const size_t index);
utl_Int256 utl_Vector_getInt256(const utl_Vector* vector, const size_t index);
double utl_Vector_getDouble(const utl_Vector* vector, const size_t index);
bool utl_Vector_getBool(const utl_Vector* vector, const size_t index);
utl_StringView utl_Vector_getBytes(const utl_Vector* vector, const size_t index);
utl_StringView utl_Vector_getString(const utl_Vector* vector, const size_t index);
struct utl_Message* utl_Vector_getMessage(const utl_Vector* vector, const size_t index);
utl_Vector* utl_Vector_getVector(const utl_Vector* vector, const size_t index);
