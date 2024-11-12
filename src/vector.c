#include "vector.h"

#include <string.h>

#define VECTOR_CAPACITY_OFFSET sizeof(size_t)
#define VECTOR_ARENA_OFFSET (VECTOR_CAPACITY_OFFSET + sizeof(void*))
#define VECTOR_REAL_START VECTOR_ARENA_OFFSET
#define VECTOR_SIZE(ELEMENTS) sizeof(size_t) + sizeof(void*) * (ELEMENTS + 1)

utl_Container* utl_Vector_new(arena_t* arena, size_t initial_size) {
    utl_Container* vector = arena_alloc(arena, sizeof(utl_Container));
    vector->size = 0;
    vector->value = arena_alloc(arena, VECTOR_SIZE(initial_size)) + VECTOR_REAL_START;
    ((arena_t**)vector->value - VECTOR_ARENA_OFFSET)[0] = arena;
    ((size_t*)vector->value - VECTOR_CAPACITY_OFFSET)[0] = initial_size;
    return vector;
}

size_t utl_Vector_capacity(utl_Container* vector) {
    return ((size_t*)vector->value)[0];
}

arena_t* utl_Vector_arena(utl_Container* vector) {
    return ((arena_t**)vector->value-sizeof(void*))[0];
}

void utl_Vector_resize(utl_Container* vector, bool force) {
    size_t capacity = utl_Vector_capacity(vector);

    if(force || vector->size >= capacity) {
        arena_t* arena = utl_Vector_arena(vector);
        void* old_value = vector->value;

        size_t new_capacity = capacity * 1.25 + 1;
        size_t new_size = VECTOR_SIZE(new_capacity);
        vector->value = arena_alloc(arena, new_size) + VECTOR_REAL_START;
        memcpy(vector->value-VECTOR_REAL_START, old_value-VECTOR_REAL_START, new_size);
        ((size_t*)vector->value - VECTOR_CAPACITY_OFFSET)[0] = new_capacity;
    }
}

void utl_Vector_append(utl_Container* vector, void* element) {
    utl_Vector_resize(vector, false);

    ((void**)vector->value)[vector->size++] = element;
}

void utl_Vector_setValue(utl_Container* vector, size_t index, void* element) {
    if(index >= vector->size) {
        return;
    }

    ((void**)vector->value)[index] = element;
}

void* utl_Vector_value(utl_Container* vector, size_t index) {
    if(index >= vector->size) {
        return 0;
    }

    return ((void**)vector->value)[index];
}

size_t utl_Vector_size(utl_Container* vector) {
    return vector->size;
}

