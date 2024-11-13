#pragma once

#include <stdint.h>

#include "arena.h"
#include "string_view.h"
#include "field_def.h"
#include "enums.h"
#include "builtins.h"

typedef struct utl_MessageDef {
    uint32_t id;
    utl_StringView name;
    utl_StringView namespace_;
    utl_StringView type;
    utl_StringView typespace;
    utl_MessageSection section;
    bool has_optional;
    uint16_t layer;
    uint16_t fields_num;
    utl_FieldDef* fields;
} utl_MessageDef;

utl_MessageDef* utl_MessageDef_new(arena_t* arena);