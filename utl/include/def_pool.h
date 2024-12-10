#pragma once

#include <stdbool.h>

#include "type_def.h"
#include "static_map.h"

#define START_POOL_SIZE DEFAULT_MAP_SIZE

typedef struct utl_DefPool {
    utl_Arena arena;
    utl_StaticMap* message_defs; // Map of utl_MessageDef
    utl_StaticMap* types; // Map of utl_TypeDef
} utl_DefPool;

utl_DefPool* utl_DefPool_new();
void utl_DefPool_free(utl_DefPool* pool);
struct utl_MessageDef* utl_DefPool_getMessage(const utl_DefPool* pool, uint32_t tl_id);
bool utl_DefPool_hasMessage(const utl_DefPool* pool, uint32_t tl_id);
void utl_DefPool_addMessage(const utl_DefPool* pool, struct utl_MessageDef* message);
void utl_DefPool_removeMessage(const utl_DefPool* pool, uint32_t tl_id);

utl_TypeDef* utl_DefPool_getType(const utl_DefPool* pool, utl_StringView name);
bool utl_DefPool_hasType(const utl_DefPool* pool, utl_StringView name);
void utl_DefPool_addType(const utl_DefPool* pool, utl_TypeDef* type);
void utl_DefPool_removeType(const utl_DefPool* pool, utl_StringView name);
