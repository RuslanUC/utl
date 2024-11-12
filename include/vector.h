#pragma once

#include "builtins.h"

utl_Container* utl_Vector_new(arena_t* arena, size_t initial_size);
size_t utl_Vector_capacity(utl_Container* vector);
void utl_Vector_append(utl_Container* vector, void* element);
void utl_Vector_setValue(utl_Container* vector, size_t index, void* element);
void* utl_Vector_value(utl_Container* vector, size_t index);
size_t utl_Vector_size(utl_Container* vector);