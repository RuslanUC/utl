#pragma once

#include <stdint.h>

#include "arena.h"
#include "string_view.h"
#include "field_def.h"
#include "type_def.h"
#include "enums.h"

typedef struct utl_MessageDef {
    uint32_t id;
    uint16_t layer;
    uint16_t fields_num;
    utl_StringView name;
    utl_StringView namespace_;
    utl_TypeDef* type;
    utl_MessageSection section;
    utl_FieldDef* fields;
    uint8_t flags_num;
    uint8_t strings_num;
    uint8_t objects_num;
    uint8_t vectors_num;
    uint8_t fully_static : 1;
    // TODO: change to u16 : 15?
    //  thats 32768 bytes, or 1024 int256 fields, or 4096 tlobject fields;
    uint32_t size : 31;
    utl_FieldDef** flags_fields;
    utl_FieldDef** string_fields;
    utl_FieldDef** object_fields;
    utl_FieldDef** vector_fields;
} utl_MessageDef;

// NOTE: If refactoring this structure, keep in mind, that .type and .sub should have same offsets as ones in utl_FieldDef
typedef struct utl_MessageDefVector {
    utl_FieldType type;
    union {
        struct utl_TypeDef* type_def;
        struct utl_MessageDefVector* vector_def;
    } sub;
    // TODO: change to uint8_t
    size_t element_size;
} utl_MessageDefVector;

utl_MessageDef* utl_MessageDef_new(utl_Arena* arena);
utl_MessageDefVector* utl_MessageDefVector_new(utl_Arena* arena);