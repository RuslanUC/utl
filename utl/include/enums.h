#pragma once

typedef enum utl_MessageSection {
    TYPES = 0,
    FUNCTIONS,
} utl_MessageSection;

typedef enum utl_FieldType {
    INT32 = 0,
    FLAGS,
    INT64,
    INT128,
    INT256,
    DOUBLE,
    FULL_BOOL,
    BIT_BOOL,

    STATIC_FIELDS_END,

    BYTES,
    STRING,
    TLOBJECT,
    VECTOR,
} utl_FieldType;