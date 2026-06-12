#pragma once

#include <stdio.h>

#ifndef NDEBUG
extern int utl_debug_logging;
extern int utl_logging_indent_level;
#endif

#ifndef NDEBUG
#    define _UTL_LOG(...) do { \
if(utl_debug_logging) { \
fprintf(stderr, "%*s", utl_logging_indent_level * 2, ""); \
fprintf(stderr, __VA_ARGS__); \
fprintf(stderr, "\n"); \
} \
} while(0)
#    define _UTL_LOG_INCIND() do { ++utl_logging_indent_level; } while(0)
#    define _UTL_LOG_DECIND() do { --utl_logging_indent_level; } while(0)
#else
#    define _UTL_LOG(...)
#    define _UTL_LOG_INCIND()
#    define _UTL_LOG_DECIND()
#endif
