#pragma once

#include <stdint.h>
#include "string_view.h"

typedef struct utl_Int128 {
    char value[16];
} utl_Int128;

typedef struct utl_Int256 {
    char value[32];
} utl_Int256;
