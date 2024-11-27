#include "string_view.h"

#include <stdlib.h>
#include <string.h>

utl_StringView utl_StringView_new(arena_t* arena, size_t size)  {
    utl_StringView string = {
        .size = size,
        .data = arena_alloc(arena, size),
    };

    return string;
}

utl_StringView utl_StringView_clone(arena_t* arena, utl_StringView src) {
    utl_StringView string = {
        .size = src.size,
        .data = arena_alloc(arena, src.size),
    };

    memcpy(string.data, src.data, src.size);
    return string;
}

utl_StringView utl_StringView_new_malloc(size_t size)  {
    utl_StringView string = {
        .size = size,
        .data = malloc(size),
    };

    return string;
}

bool utl_StringView_equals(utl_StringView a, utl_StringView b) {
    if(a.data == b.data) {
        return true;
    }
    if(a.size != b.size) {
        return false;
    }

    return !memcmp(a.data, b.data, a.size);
}