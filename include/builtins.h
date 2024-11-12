#pragma once

#include "string_view.h"

typedef struct utl_Int32 {
    int32_t value;
} utl_Int32;

typedef struct utl_Int64 {
    int64_t value;
} utl_Int64;

typedef struct utl_Int128 {
    char value[16];
} utl_Int128;

typedef struct utl_Int256 {
    char value[32];
} utl_Int256;

typedef struct utl_Double {
    double value;
} utl_Double;

typedef struct utl_Bool {
    bool value;
} utl_Bool;

typedef struct utl_Bytes {
    size_t max_size;
    utl_StringView value;
} utl_Bytes;

typedef struct utl_String {
    size_t max_size;
    utl_StringView value;
} utl_String;

typedef struct utl_Container {
    uint32_t tl_id; // TODO: is it needed?
    // In case of Vector - size is a number of elements, value is an array of pointers to utl_* structs,
    // If case of TLObject - size is a size of a TLObject, value is a pointer to utl_Message.
    size_t size;
    void* value;
} utl_Container;