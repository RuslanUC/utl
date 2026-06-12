#include "arena_static_refcounted.h"

#include <stdlib.h>

#include "logging.h"

utl_StaticRefCntArena* utl_StaticRefCntArena_new(const size_t size) {
    utl_StaticRefCntArena* arena = malloc(sizeof(utl_StaticRefCntArena) + size);
    arena->base = arena->data = (uint8_t*)(arena + 1);
    arena->size = size;
    arena->ref_count = 0;
    return arena;
}

void* utl_StaticRefCntArena_alloc(utl_StaticRefCntArena* arena, const size_t n) {
    if(arena->data - arena->base + n > arena->size)
        return NULL;
    void* result = arena->data;
    arena->data += n;
    arena->ref_count++;
    _UTL_LOG("ARENA_INCREF(%p) = %d", arena, arena->ref_count);
    return result;
}

void utl_StaticRefCntArena_incref(utl_StaticRefCntArena* arena) {
    _UTL_LOG("ARENA_INCREF(%p) = %d", arena, arena->ref_count);
    arena->ref_count++;
}

void utl_StaticRefCntArena_decref(utl_StaticRefCntArena* arena) {
    _UTL_LOG("ARENA_DECREF(%p) = %d", arena, arena->ref_count - 1);
    if(--arena->ref_count <= 0) {
        _UTL_LOG("ARENA_FREE(%p)", arena);
        free(arena);
    }
}

