#pragma once

#include "builtins.h"
#include "message_def.h"

typedef struct utl_Vector {
    arena_t arena;
    utl_MessageDefVector* message_def;
    size_t size;
    size_t capacity;
    void** items;
} utl_Vector;

utl_Vector* utl_Vector_new(utl_MessageDefVector* vector_def, size_t initial_size);
void utl_Vector_free(utl_Vector* vector);
size_t utl_Vector_capacity(const utl_Vector* vector);
void utl_Vector_append(utl_Vector* vector, void* element);
void utl_Vector_setValue(const utl_Vector* vector, size_t index, void* element);
void utl_Vector_remove(utl_Vector* vector, size_t index);
void* utl_Vector_value(const utl_Vector* vector, size_t index);
size_t utl_Vector_size(const utl_Vector* vector);
bool utl_Vector_equals(const utl_Vector* a, const utl_Vector* b);
