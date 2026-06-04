#include "def_pool.h"

#include <stdlib.h>
#include "stb_ds.h"
#include "message_def.h"

typedef struct utl_TlIdMessageDefPair_s {
    uint32_t key;
    utl_MessageDef* value;
} utl_TlIdMessageDefPair;

typedef struct utl_NameTypeDefPair_s {
    uint32_t key;
    utl_TypeDef* value;
} utl_NameTypeDefPair;

utl_DefPool* utl_DefPool_new() {
    utl_DefPool* result = malloc(sizeof(utl_DefPool));
    result->arena = utl_Arena_new(128 * 1024);
    result->message_defs = NULL;
    result->types = NULL;

    return result;
}

void utl_DefPool_free(utl_DefPool* pool) {
    utl_Arena_free(&pool->arena);
    hmfree(pool->message_defs);
    hmfree(pool->types);
    free(pool);
}

utl_MessageDef* utl_DefPool_getMessage(utl_DefPool* pool, const uint32_t tl_id) {
    const ptrdiff_t idx = hmgeti(pool->message_defs, tl_id);
    if(idx >= 0)
        return pool->message_defs[idx].value;
    return NULL;
}

bool utl_DefPool_hasMessage(utl_DefPool* pool, const uint32_t tl_id) {
    return hmgeti(pool->message_defs, tl_id) >= 0;
}

void utl_DefPool_addMessage(utl_DefPool* pool, utl_MessageDef* message) {
    hmput(pool->message_defs, message->id, message);
    utl_DefPool_addType(pool, message->type);
}

void utl_DefPool_removeMessage(utl_DefPool* pool, const uint32_t tl_id) {
    hmdel(pool->message_defs, tl_id);
}

utl_TypeDef* utl_DefPool_getType(utl_DefPool* pool, const utl_StringView name) {
    utl_Arena_state save = {0};
    utl_Arena_save(&pool->arena, &save);

    const utl_StringView tmp = utl_StringView_clone(&pool->arena, name);
    const ptrdiff_t idx = shgeti(pool->types, tmp.data);

    utl_Arena_restore(&pool->arena, save);
    if(idx >= 0)
        return pool->types[idx].value;
    return NULL;
}

bool utl_DefPool_hasType(utl_DefPool* pool, const utl_StringView name) {
    utl_Arena_state save = {0};
    utl_Arena_save(&pool->arena, &save);

    const utl_StringView tmp = utl_StringView_clone(&pool->arena, name);
    const int result = shgeti(pool->types, tmp.data) >= 0;

    utl_Arena_restore(&pool->arena, save);
    return result;
}

void utl_DefPool_addType(utl_DefPool* pool, utl_TypeDef* type) {
    const utl_StringView tmp = utl_StringView_clone(&pool->arena, type->name);
    shput(pool->types, tmp.data, type);
}

void utl_DefPool_removeType(utl_DefPool* pool, const utl_StringView name) {
    utl_Arena_state save = {0};
    utl_Arena_save(&pool->arena, &save);

    const utl_StringView tmp = utl_StringView_clone(&pool->arena, name);
    shdel(pool->types, tmp.data);

    utl_Arena_restore(&pool->arena, save);
}
