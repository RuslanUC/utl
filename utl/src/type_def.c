#include "type_def.h"

utl_TypeDef* utl_TypeDef_new(utl_Arena* arena) {
    return utl_Arena_alloc(arena, sizeof(utl_TypeDef));
}
