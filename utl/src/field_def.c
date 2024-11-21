#include "field_def.h"

utl_FieldDef* utl_FieldDef_new(arena_t* arena) {
    return arena_alloc(arena, sizeof(utl_FieldDef));
}
