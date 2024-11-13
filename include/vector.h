#pragma once

#include "builtins.h"
#include "message.h"

typedef struct utl_Vector {
    arena_t arena;
    utl_MessageDef* message_def;
    size_t size;
    size_t capacity;
    void** items;
} utl_Vector;

utl_Vector* utl_Vector_new(size_t initial_size);
void utl_Vector_free(utl_Vector* vector);
size_t utl_Vector_capacity(utl_Vector* vector);
void utl_Vector_append(utl_Vector* vector, void* element);
void utl_Vector_setValue(utl_Vector* vector, size_t index, void* element);
void* utl_Vector_value(utl_Vector* vector, size_t index);
size_t utl_Vector_size(utl_Vector* vector);