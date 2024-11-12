#pragma once
#include <arena.h>
#include <stddef.h>

typedef struct utl_StringView {
    size_t size;
    char* data;
} utl_StringView;

utl_StringView utl_StringView_new(arena_t* arena, size_t size);
