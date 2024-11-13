#include "type_def.h"

utl_TypeDef* utl_TypeDef_new(arena_t* arena) {
    return arena_alloc(arena, sizeof(utl_TypeDef));
}
