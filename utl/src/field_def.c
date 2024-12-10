#include "field_def.h"

utl_FieldDef* utl_FieldDef_new(utl_Arena* arena) {
    return utl_Arena_alloc(arena, sizeof(utl_FieldDef));
}
