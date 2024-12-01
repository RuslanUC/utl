#pragma once

#include <string_view.h>
#include <map_list.h>

typedef struct utl_ListNodePtr {
    utl_ListNode base;
    void* key;
} utl_ListNodePtr;

typedef struct utl_PtrMap {
    size_t size;
    size_t buckets_num;
    utl_ListNodePtr** buckets;
} utl_PtrMap;

utl_PtrMap* utl_PtrMap_new(size_t buckets_num);
void utl_PtrMap_free(const utl_PtrMap* map);
void utl_PtrMap_insert(utl_PtrMap* map, void* key, void* value);
void utl_PtrMap_remove(utl_PtrMap* map, void* key);
void* utl_PtrMap_search(const utl_PtrMap* map, void* key);
