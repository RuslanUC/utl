#pragma once

#include "map_list.h"

typedef struct utl_Map {
    arena_t arena;
    size_t buckets_num;
    utl_ListNode** buckets;
} utl_Map;

utl_Map* utl_Map_new(arena_t* arena, size_t buckets_num);
void utl_Map_free(utl_Map* map);
void utl_Map_insert(utl_Map* map, uint32_t key, void* value);
void utl_Map_remove(utl_Map* map, uint32_t key);
void* utl_Map_search(utl_Map* map, uint32_t key);
