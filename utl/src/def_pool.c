#include "def_pool.h"

#include <stdlib.h>
#include "message_def.h"

utl_DefPool* utl_DefPool_new() {
    utl_DefPool* result = malloc(sizeof(utl_DefPool));
    result->arena = utl_Arena_new(4096);
    result->message_defs = utl_Map_new(START_POOL_SIZE);
    result->types = utl_Map_new(START_POOL_SIZE);

    return result;
}

void utl_DefPool_free(utl_DefPool* pool) {
    utl_Arena_free(&pool->arena);
    free(pool);
}

utl_MessageDef* utl_DefPool_getMessage(const utl_DefPool* pool, const uint32_t tl_id) {
    return utl_Map_search(pool->message_defs, tl_id);
}

bool utl_DefPool_hasMessage(const utl_DefPool* pool, const uint32_t tl_id) {
    return utl_DefPool_getMessage(pool, tl_id) != 0;
}

void utl_DefPool_addMessage(const utl_DefPool* pool, utl_MessageDef* message) {
    utl_Map_insert(pool->message_defs, message->id, message);
    utl_DefPool_addType(pool, message->type);
}

void utl_DefPool_removeMessage(const utl_DefPool* pool, const uint32_t tl_id) {
    utl_Map_remove(pool->message_defs, tl_id);
}

utl_TypeDef* utl_DefPool_getType(const utl_DefPool* pool, const utl_StringView name) {
    return utl_Map_search_str(pool->types, name);
}

bool utl_DefPool_hasType(const utl_DefPool* pool, const utl_StringView name) {
    return utl_DefPool_getType(pool, name) != 0;
}

void utl_DefPool_addType(const utl_DefPool* pool, utl_TypeDef* type) {
    utl_Map_insert_str(pool->types, type->name, type);
}

void utl_DefPool_removeType(const utl_DefPool* pool, const utl_StringView name) {
    utl_Map_remove_str(pool->types, name);
}
