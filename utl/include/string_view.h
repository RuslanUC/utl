#pragma once
#include <arena.h>
#include <stddef.h>

typedef struct utl_StringView {
    size_t size;
    char* data;
} utl_StringView;

utl_StringView utl_StringView_new(arena_t* arena, size_t size);
utl_StringView utl_StringView_clone(arena_t* arena, utl_StringView src);
utl_StringView utl_StringView_new_malloc(size_t size);

bool utl_StringView_equals(utl_StringView a, utl_StringView b);
