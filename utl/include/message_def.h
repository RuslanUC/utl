#pragma once

#include <stdint.h>

#include "arena.h"
#include "string_view.h"
#include "field_def.h"
#include "type_def.h"
#include "enums.h"

typedef struct utl_MessageDef {
    uint32_t id;
    utl_StringView name;
    utl_StringView namespace_;
    utl_TypeDef* type;
    utl_MessageSection section;
    uint16_t layer;
    uint16_t fields_num;
    utl_FieldDef* fields;
    uint8_t flags_num;
    utl_FieldDef** flags_fields;
} utl_MessageDef;

typedef struct utl_MessageDefVector {
    utl_FieldType type;
    union {
        struct utl_TypeDef* type_def;
        struct utl_MessageDefVector* vector_def;
    } sub;
} utl_MessageDefVector;

utl_MessageDef* utl_MessageDef_new(arena_t* arena);
utl_MessageDefVector* utl_MessageDefVector_new(arena_t* arena);