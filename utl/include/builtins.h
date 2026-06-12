#pragma once

typedef struct utl_Int128 {
    char value[16];
} utl_Int128;

typedef struct utl_Int256 {
    char value[32];
} utl_Int256;

_Static_assert(sizeof(utl_Int128) == 16, "utl_Int128 must be exactly 16 bytes");
_Static_assert(sizeof(utl_Int256) == 32, "utl_Int256 must be exactly 32 bytes");
