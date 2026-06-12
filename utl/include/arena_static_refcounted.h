#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct utl_StaticRefCntArena {
    uint8_t* base;
    uint8_t* data;
    size_t size;
    int32_t ref_count;
} utl_StaticRefCntArena;

utl_StaticRefCntArena* utl_StaticRefCntArena_new(size_t size);
void* utl_StaticRefCntArena_alloc(utl_StaticRefCntArena* arena, size_t n);
void utl_StaticRefCntArena_incref(utl_StaticRefCntArena* arena);
void utl_StaticRefCntArena_decref(utl_StaticRefCntArena* arena);
