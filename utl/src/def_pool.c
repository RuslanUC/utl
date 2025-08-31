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
    result->arena = utl_Arena_new(4096);
    result->message_defs = NULL;
    result->types = NULL;
    result->tmp_string_buffer.capacity = 0;
    result->tmp_string_buffer.data = NULL;

    return result;
}

void utl_DefPool_free(utl_DefPool* pool) {
    utl_Arena_free(&pool->arena);
    hmfree(pool->message_defs);
    hmfree(pool->types);
    free(pool->tmp_string_buffer.data);
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
    hmdel(pool->message_defs,tl_id);
}

// TODO: check if stb_ds stores given pointer to the string or it copies the string and uses the new one

static void resize_tmp_string_buffer_maybe(utl_DefPool* pool, const size_t str_length) {
    const size_t need_capacity = str_length + 1;
    if(need_capacity > pool->tmp_string_buffer.capacity) {
        pool->tmp_string_buffer.capacity = need_capacity;
        pool->tmp_string_buffer.data = realloc(pool->tmp_string_buffer.data, need_capacity);
    }
}

static void copy_string_to_tmp_string_buffer(utl_DefPool* pool, const utl_StringView string) {
    resize_tmp_string_buffer_maybe(pool, string.size);
    memcpy(pool->tmp_string_buffer.data, string.data, string.size);
    pool->tmp_string_buffer.data[string.size] = '\0';
}

utl_TypeDef* utl_DefPool_getType(utl_DefPool* pool, const utl_StringView name) {
    copy_string_to_tmp_string_buffer(pool, name);

    const ptrdiff_t idx = shgeti(pool->types, pool->tmp_string_buffer.data);
    if(idx >= 0)
        return pool->types[idx].value;
    return NULL;
}

bool utl_DefPool_hasType(utl_DefPool* pool, const utl_StringView name) {
    copy_string_to_tmp_string_buffer(pool, name);
    return shgeti(pool->types, pool->tmp_string_buffer.data) >= 0;
}

void utl_DefPool_addType(utl_DefPool* pool, utl_TypeDef* type) {
    copy_string_to_tmp_string_buffer(pool, type->name);
    shput(pool->types, pool->tmp_string_buffer.data, type);
}

void utl_DefPool_removeType(utl_DefPool* pool, const utl_StringView name) {
    copy_string_to_tmp_string_buffer(pool, name);
    shdel(pool->types, pool->tmp_string_buffer.data);
}
