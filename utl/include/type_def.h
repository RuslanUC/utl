#pragma once

#include <stdint.h>

#include "arena.h"
#include "string_view.h"

struct utl_MessageDef;

typedef struct utl_TypeDef {
    utl_StringView name; // Full name (e.g. namespace.Name)
} utl_TypeDef;

utl_TypeDef* utl_TypeDef_new(arena_t* arena);