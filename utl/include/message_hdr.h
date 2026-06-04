#pragma once

#include "message_def.h"

#define UTL_MESSAGE_HEADER struct { \
        utl_MessageDef* message_def; \
        void* userdata; \
    }

typedef struct utl_MessageHeader {
    UTL_MESSAGE_HEADER;
} utl_MessageHeader;
