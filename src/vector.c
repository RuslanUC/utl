#include "vector.h"

#include <string.h>
#include <utils.h>

utl_Vector* utl_Vector_new(size_t initial_size) {
    arena_t arena = arena_new();
    utl_Vector* vector = arena_alloc(&arena, sizeof(utl_Vector));
    vector->size = 0;
    vector->capacity = initial_size;
    vector->items = arena_alloc(&arena, sizeof(void*) * initial_size);
    vector->arena = arena;
    return vector;
}

void utl_Vector_free(utl_Vector* vector) {
    arena_delete(&vector->arena);
}

size_t utl_Vector_capacity(utl_Vector* vector) {
    return vector->capacity;
}

arena_t* utl_Vector_arena(utl_Vector* vector) {
    return &vector->arena;
}

void utl_Vector_resize(utl_Vector* vector, bool force) {
    size_t capacity = utl_Vector_capacity(vector);

    if(force || vector->size >= capacity) {
        void* old_items = vector->items;
        vector->capacity = capacity * 1.25 + 1;
        vector->items = arena_alloc(&vector->arena, sizeof(void*) * vector->capacity);
        memcpy(vector->items, old_items, sizeof(void*) * vector->size);
    }
}

void utl_Vector_append(utl_Vector* vector, void* element) {
    utl_Vector_resize(vector, false);

    vector->items[vector->size++] = element;
}

void utl_Vector_setValue(utl_Vector* vector, size_t index, void* element) {
    if(index >= vector->size) {
        return;
    }

    vector->items[index] = element;
}

void* utl_Vector_value(utl_Vector* vector, size_t index) {
    if(index >= vector->size) {
        return 0;
    }

    return vector->items[index];
}

size_t utl_Vector_size(utl_Vector* vector) {
    return vector->size;
}

