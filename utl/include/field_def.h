#pragma once

#include <arena.h>
#include <stdint.h>

#include "string_view.h"
#include "enums.h"

// NOTE: If refactoring this structure, keep in mind, that .type and .sub should have same offsets as ones in utl_MessageDefVector
typedef struct utl_FieldDef {
    utl_FieldType type;
    // Vectors and TLObjects
    union {
        struct utl_TypeDef* type_def;
        struct utl_MessageDefVector* vector_def;
    } sub;

    // TODO: set to uint16_t?
    size_t num;
    size_t offset;
    utl_StringView name;
    // if fields is optional, 3 higher bits represent flag_num (flags, flags2, flags3, etc.), 5 lower bits
    //   represent flag_bit (max value is 31 since flags field type is int32)
    // if fields is not optional, flag_info is 0
    uint8_t flag_info;
} utl_FieldDef;

utl_FieldDef* utl_FieldDef_new(utl_Arena* arena);
