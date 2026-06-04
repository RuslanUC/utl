#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifndef UTL_STATUS_MAX_MESSAGE_SIZE
#define UTL_STATUS_MAX_MESSAGE_SIZE 124
#endif

typedef struct utl_Status {
    bool ok : 1;
    uint32_t pos : 31;
    char message[UTL_STATUS_MAX_MESSAGE_SIZE];
} utl_Status;
