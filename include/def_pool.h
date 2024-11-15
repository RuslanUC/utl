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
struct utl_MessageDef* utl_DefPool_get_message(utl_DefPool* pool, uint32_t tl_id);
bool utl_DefPool_has_message(utl_DefPool* pool, uint32_t tl_id);
void utl_DefPool_add_message(utl_DefPool* pool, struct utl_MessageDef* message);
void utl_DefPool_remove_message(utl_DefPool* pool, uint32_t tl_id);

utl_TypeDef* utl_DefPool_get_type(utl_DefPool* pool, utl_StringView name);
bool utl_DefPool_has_type(utl_DefPool* pool, utl_StringView name);
void utl_DefPool_add_type(utl_DefPool* pool, utl_TypeDef* type);
void utl_DefPool_remove_type(utl_DefPool* pool, utl_StringView name);
