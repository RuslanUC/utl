#pragma once

#include <stdbool.h>

#include "type_def.h"

#define START_POOL_SIZE DEFAULT_MAP_SIZE

typedef struct utl_TlIdMessageDefPair_s utl_TlIdMessageDefPair;
typedef struct utl_NameTypeDefPair_s utl_NameTypeDefPair;

typedef struct utl_DefPool {
    utl_Arena arena;
    utl_TlIdMessageDefPair* message_defs;
    utl_NameTypeDefPair* types;
    struct {
        size_t capacity;
        char* data;
    } tmp_string_buffer;
} utl_DefPool;

utl_DefPool* utl_DefPool_new();
void utl_DefPool_free(utl_DefPool* pool);
struct utl_MessageDef* utl_DefPool_getMessage(utl_DefPool* pool, uint32_t tl_id);
bool utl_DefPool_hasMessage(utl_DefPool* pool, uint32_t tl_id);
void utl_DefPool_addMessage(utl_DefPool* pool, struct utl_MessageDef* message);
void utl_DefPool_removeMessage(utl_DefPool* pool, uint32_t tl_id);

utl_TypeDef* utl_DefPool_getType(utl_DefPool* pool, utl_StringView name);
bool utl_DefPool_hasType(utl_DefPool* pool, utl_StringView name);
void utl_DefPool_addType(utl_DefPool* pool, utl_TypeDef* type);
void utl_DefPool_removeType(utl_DefPool* pool, utl_StringView name);
