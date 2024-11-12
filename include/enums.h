#pragma once

typedef enum utl_MessageSection {
    TYPES = 0,
    FUNCTIONS,
} utl_MessageSection;

typedef enum utl_FieldType {
    INT32 = 0,
    INT64,
    INT128,
    INT256,
    DOUBLE,
    BOOL,
    BYTES,
    STRING,
    TLOBJECT,
    VECTOR,
} utl_FieldType;