#pragma once

#include <type_def.h>

#include "static_map.h"

#define START_POOL_SIZE 1024

typedef struct utl_DefPool {
    arena_t arena;
    utl_StaticMap* message_defs; // Map of utl_MessageDef
    utl_StaticMap* types; // Map of utl_TypeDef
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
