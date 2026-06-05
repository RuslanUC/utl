#pragma once

#include <stdint.h>

#ifndef UTL_SCRATCH_ALLOC_SAVE_POINTS_NUM
#   define UTL_SCRATCH_ALLOC_SAVE_POINTS_NUM 4
#endif
#ifndef UTL_SCRATCH_ALLOC_DEFAULT_CAPACITY
#   define UTL_SCRATCH_ALLOC_DEFAULT_CAPACITY (128 * 1024)
#endif

typedef struct utl_ScratchAlloc {
    uint8_t* data;
    uint32_t size;
    uint32_t capacity;
} utl_ScratchAlloc;

utl_ScratchAlloc utl_ScratchAlloc_init();
utl_ScratchAlloc utl_ScratchAlloc_init_with_capacity(uint32_t capacity);
void* utl_ScratchAlloc_alloc(utl_ScratchAlloc* alloc, uint32_t n);
void utl_ScratchAlloc_reset(utl_ScratchAlloc* alloc);
uint32_t utl_ScratchAlloc_save(utl_ScratchAlloc* alloc);
void utl_ScratchAlloc_restore(utl_ScratchAlloc* alloc, uint32_t save_point);
void utl_ScratchAlloc_free(utl_ScratchAlloc* alloc);


