#pragma once

#include "message.h"

static const char BOOL_TRUE[4] = {(char)0x99, (char)0x72, (char)0x75, (char)0xb5};
static const char BOOL_FALSE[4] = {(char)0xbc, (char)0x79, (char)0x97, (char)0x37};
static const uint32_t VECTOR_CONSTR = 0x1cb5c415;

typedef struct utl_EncodeBuf {
    char* data;
    size_t pos;
    size_t size;
} utl_EncodeBuf;

char* utl_EncodeBuf_alloc(utl_EncodeBuf* buf, size_t n_bytes);
char* utl_encode(const utl_Message* message, size_t* out_size);