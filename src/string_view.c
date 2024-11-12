#include "string_view.h"

utl_StringView utl_StringView_new(arena_t* arena, size_t size)  {
    utl_StringView string = {
        .size = size,
        .data = arena_alloc(arena, size),
    };

    return string;
}