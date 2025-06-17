#pragma once

#include <stdbool.h>

#include "message_def.h"
#include "builtins.h"

struct utl_RoMessage;

typedef struct utl_RoVector {
    utl_Arena arena;
    utl_MessageDefVector* message_def;
    size_t size;
    size_t capacity;
    void* data;
} utl_RoVector;

utl_RoVector* utl_RoVector_new(utl_MessageDefVector* vector_def, size_t initial_size);
void utl_RoVector_free(utl_RoVector* vector);
size_t utl_RoVector_capacity(const utl_RoVector* vector);
size_t utl_RoVector_size(const utl_RoVector* vector);
bool utl_RoVector_equals(const utl_RoVector* a, const utl_RoVector* b);
void* utl_RoVector_rawValue(const utl_RoVector* vector, size_t index);

int32_t utl_RoVector_getInt32(const utl_RoVector* vector, size_t index);
int64_t utl_RoVector_getInt64(const utl_RoVector* vector, size_t index);
utl_Int128 utl_RoVector_getInt128(const utl_RoVector* vector, size_t index);
utl_Int256 utl_RoVector_getInt256(const utl_RoVector* vector, size_t index);
double utl_RoVector_getDouble(const utl_RoVector* vector, size_t index);
bool utl_RoVector_getBool(const utl_RoVector* vector, size_t index);
utl_StringView utl_RoVector_getBytes(const utl_RoVector* vector, size_t index);
utl_StringView utl_RoVector_getString(const utl_RoVector* vector, size_t index);
struct utl_Message* utl_RoVector_getMessage(const utl_RoVector* vector, size_t index);
utl_RoVector* utl_RoVector_getVector(const utl_RoVector* vector, size_t index);
