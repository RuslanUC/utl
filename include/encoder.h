#pragma once

#include "utils.h"
#include "message.h"

static const char BOOL_TRUE[4] = {(char)0x99, (char)0x72, (char)0x75, (char)0xb5};
static const char BOOL_FALSE[4] = {(char)0xbc, (char)0x79, (char)0x97, (char)0x37};

size_t utl_encode(const utl_Message* message, arena_t* arena);