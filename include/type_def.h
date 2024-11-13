#pragma once

#include <stdint.h>

#include "arena.h"
#include "string_view.h"

struct utl_MessageDef;

typedef struct utl_TypeDef {
    utl_StringView name;
    utl_StringView namespace_;
    uint16_t message_defs_num;
    struct utl_MessageDef** message_defs;
} utl_TypeDef;

utl_TypeDef* utl_TypeDef_new(arena_t* arena);