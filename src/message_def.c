#include "message_def.h"

utl_MessageDef* utl_MessageDef_new(arena_t* arena) {
    return arena_alloc(arena, sizeof(utl_MessageDef));
}

utl_MessageDefVector* utl_MessageDefVector_new(arena_t* arena) {
    return arena_alloc(arena, sizeof(utl_MessageDefVector));
}