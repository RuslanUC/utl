#pragma once

#include "utils.h"
#include "message.h"
#include "def_pool.h"

typedef struct utl_DecodeBuf {
    char* data;
    size_t pos;
    size_t size;
} utl_DecodeBuf;

int32_t utl_decode_int32(utl_DecodeBuf* buffer);
int64_t utl_decode_int64(utl_DecodeBuf* buffer);
void utl_decode_int128(char* out, utl_DecodeBuf* buffer);
void utl_decode_int256(char* out, utl_DecodeBuf* buffer);
double utl_decode_double(utl_DecodeBuf* buffer);
bool utl_decode_bool(utl_DecodeBuf* buffer);
utl_StringView utl_decode_bytes(utl_DecodeBuf* buffer, arena_t* arena);

size_t utl_decode(utl_Message* out_message, utl_DefPool* def_pool, char* buf, size_t size);