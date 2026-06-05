#include "scratch_alloc.h"

#include <stdlib.h>

utl_ScratchAlloc utl_ScratchAlloc_init() {
    return utl_ScratchAlloc_init_with_capacity(UTL_SCRATCH_ALLOC_DEFAULT_CAPACITY);
}

utl_ScratchAlloc utl_ScratchAlloc_init_with_capacity(const uint32_t capacity) {
    return (utl_ScratchAlloc){
        .data = malloc(capacity),
        .size = 0,
        .capacity = capacity,
    };
}

void* utl_ScratchAlloc_alloc(utl_ScratchAlloc* alloc, const uint32_t n) {
    if(alloc->data == NULL)
        alloc->data = malloc(alloc->capacity);
    if(alloc->size + n >= alloc->capacity)
        return NULL;
    void* ret = alloc->data + alloc->size;
    alloc->size += n;
    return ret;
}

void utl_ScratchAlloc_reset(utl_ScratchAlloc* alloc) {
    alloc->size = 0;
}

uint32_t utl_ScratchAlloc_save(utl_ScratchAlloc* alloc) {
    return alloc->size;
}

void utl_ScratchAlloc_restore(utl_ScratchAlloc* alloc, const uint32_t save_point) {
    alloc->size = save_point;
}

void utl_ScratchAlloc_free(utl_ScratchAlloc* alloc) {
    free(alloc->data);
    alloc->data = NULL;
    alloc->size = 0;
}