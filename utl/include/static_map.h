#pragma once

#include "string_view.h"
#include "map_list.h"

#define DEFAULT_MAP_SIZE 1024

typedef struct utl_StaticMap {
    uint8_t arena_type; // 0 is owned arena struct, 1 is "borrowed" arena pointer
    union {
        utl_Arena arena;
        utl_Arena* arena_ptr;
    } arena;
    size_t buckets_num;
    utl_ListNode* buckets[DEFAULT_MAP_SIZE];
} utl_StaticMap;

utl_StaticMap* utl_Map_new(size_t buckets_num);
utl_StaticMap* utl_Map_new_on_arena(size_t buckets_num, utl_Arena* arena);
void utl_Map_free(utl_StaticMap* map);
void utl_Map_insert(utl_StaticMap* map, uint32_t key, void* value);
void utl_Map_remove(utl_StaticMap* map, uint32_t key);
void* utl_Map_search(const utl_StaticMap* map, uint32_t key);

void utl_Map_insert_str(utl_StaticMap* map, utl_StringView key, void* value);
void utl_Map_remove_str(utl_StaticMap* map, utl_StringView key);
void* utl_Map_search_str(const utl_StaticMap* map, utl_StringView key);

void utl_Map_insert_uint64(utl_StaticMap* map, uint64_t key, void* value);
void utl_Map_remove_uint64(utl_StaticMap* map, uint64_t key);
void* utl_Map_search_uint64(const utl_StaticMap* map, uint64_t key);
