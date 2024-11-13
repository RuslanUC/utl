#pragma once

#include <arena.h>
#include <stdint.h>

#include "arena.h"
#include "string_view.h"
#include "enums.h"

struct utl_MessageDefBase;

typedef struct utl_FieldDef {
    size_t num;
    // TODO: add name
    utl_FieldType type;
    // if fields is optional, 3 higher bits represent flag_num (flags, flags2, flags3, etc.), 5 lower bits
    //   represent flag_bit (max value is 32 since flags field type is int32)
    // if fields is not optional, flag_info is 0
    uint8_t flag_info;
    // Vectors and TLObjects
    struct utl_MessageDefBase* sub_message_def;  // utl_TypeDef, utl_MessageDefVector or NULL
} utl_FieldDef;

utl_FieldDef* utl_FieldDef_new(arena_t* arena);
