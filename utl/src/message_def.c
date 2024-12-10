#include "message_def.h"

utl_MessageDef* utl_MessageDef_new(utl_Arena* arena) {
    return utl_Arena_alloc(arena, sizeof(utl_MessageDef));
}

utl_MessageDefVector* utl_MessageDefVector_new(utl_Arena* arena) {
    return utl_Arena_alloc(arena, sizeof(utl_MessageDefVector));
}