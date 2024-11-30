#pragma once

#include <arena.h>
#include <stdint.h>

#include "string_view.h"
#include "enums.h"

typedef struct utl_FieldDef {
    size_t num;
    utl_StringView name;
    utl_FieldType type;
    // if fields is optional, 3 higher bits represent flag_num (flags, flags2, flags3, etc.), 5 lower bits
    //   represent flag_bit (max value is 31 since flags field type is int32)
    // if fields is not optional, flag_info is 0
    uint8_t flag_info;
    // Vectors and TLObjects
    union {
        struct utl_TypeDef* type_def;
        struct utl_MessageDefVector* vector_def;
    } sub;
} utl_FieldDef;

utl_FieldDef* utl_FieldDef_new(arena_t* arena);
