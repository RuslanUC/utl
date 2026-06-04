#pragma once

#include "message_def.h"

#define UTL_VECTOR_HEADER struct { \
        utl_MessageDefVector* message_def; \
        void* userdata; \
    }

typedef struct utl_VectorHeader {
    UTL_VECTOR_HEADER;
} utl_VectorHeader;
