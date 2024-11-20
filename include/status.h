#pragma once
#include <stdbool.h>

#ifndef UTL_STATUS_MAX_MESSAGE_SIZE
#define UTL_STATUS_MAX_MESSAGE_SIZE 127
#endif

typedef struct utl_Status {
    bool ok;
    char message[UTL_STATUS_MAX_MESSAGE_SIZE];
} utl_Status;
