#pragma once

#include "message.h"
#include "def_pool.h"
#include "status.h"

typedef struct utl_DecodeBuf {
    uint8_t* data;
    size_t pos;
    size_t size;
} utl_DecodeBuf;

uint8_t* utl_DecodeBuf_read(utl_DecodeBuf* buf, size_t n);
uint8_t* utl_DecodeBuf_read_with_oef_check(utl_DecodeBuf* buf, size_t n);

int32_t utl_decode_int32(utl_DecodeBuf* buffer);
int64_t utl_decode_int64(utl_DecodeBuf* buffer);
void utl_decode_int128(char* out, utl_DecodeBuf* buffer);
void utl_decode_int256(char* out, utl_DecodeBuf* buffer);
double utl_decode_double(utl_DecodeBuf* buffer);
bool utl_decode_bool(utl_DecodeBuf* buffer);
utl_StringView utl_decode_bytes(utl_DecodeBuf* buffer, utl_Arena* arena, utl_Status* status);

size_t utl_decode(utl_Message* out_message, utl_DefPool* def_pool, uint8_t* buf, size_t size, utl_Status* status);