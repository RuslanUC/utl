#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "string_view.h"

utl_StringView utl_StringView_new(utl_Arena* arena, const size_t size)  {
    const utl_StringView string = {
        .size = size,
        .data = utl_Arena_alloc(arena, size),
    };

    return string;
}

utl_StringView utl_StringView_clone(utl_Arena* arena, const utl_StringView src) {
    const utl_StringView string = {
        .size = src.size,
        .data = utl_Arena_alloc(arena, src.size),
    };

    memcpy(string.data, src.data, src.size);
    return string;
}

utl_StringView utl_StringView_new_malloc(const size_t size)  {
    const utl_StringView string = {
        .size = size,
        .data = malloc(size),
    };

    return string;
}

bool utl_StringView_equals(const utl_StringView a, const utl_StringView b) {
    if(a.data == b.data) {
        return true;
    }
    if(a.size != b.size) {
        return false;
    }

    return !memcmp(a.data, b.data, a.size);
}