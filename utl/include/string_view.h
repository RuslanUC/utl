#pragma once
#include <arena.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct utl_StringView {
    size_t size;
    char* data;
} utl_StringView;

utl_StringView utl_StringView_new(utl_Arena* arena, size_t size);
utl_StringView utl_StringView_clone(utl_Arena* arena, utl_StringView src);
utl_StringView utl_StringView_new_malloc(size_t size);

bool utl_StringView_equals(utl_StringView a, utl_StringView b);
