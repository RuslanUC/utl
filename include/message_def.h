#pragma once

#include <stdint.h>

#include "arena.h"
#include "string_view.h"
#include "field_def.h"
#include "type_def.h"
#include "enums.h"
#include "builtins.h"

typedef struct utl_MessageDefBase {} utl_MessageDefBase;

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
    // TODO: maybe make it utl_FieldDef**, so it will point to existing field
    utl_FieldDef* flags_fields;
} utl_MessageDef;

typedef struct utl_MessageDefVector {
    utl_FieldType type;
    utl_MessageDefBase* sub_message_def;  // utl_TypeDef, utl_MessageDefVector or NULL
} utl_MessageDefVector;

utl_MessageDef* utl_MessageDef_new(arena_t* arena);
utl_MessageDefVector* utl_MessageDefVector_new(arena_t* arena);