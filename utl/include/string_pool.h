#pragma once

#include "string_view.h"

utl_StringView utl_StringPool_alloc(size_t size);
void utl_StringPool_free(utl_StringView string);
utl_StringView utl_StringPool_realloc(utl_StringView string, size_t size);