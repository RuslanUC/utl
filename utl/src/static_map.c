#include "static_map.h"

#include <string.h>
#include <utils.h>

utl_StaticMap* utl_Map_new(size_t buckets_num) {
    arena_t arena = arena_new();
    utl_StaticMap* map = arena_alloc(&arena, sizeof(utl_StaticMap));
    map->arena_type = 0;
    map->arena.arena = arena;
    map->buckets_num = buckets_num;
    map->buckets = arena_alloc(&map->arena.arena, sizeof(utl_ListNode*) * buckets_num);
    memset(map->buckets, 0, sizeof(utl_ListNode*) * map->buckets_num);

    return map;
}

utl_StaticMap* utl_Map_new_on_arena(size_t buckets_num, arena_t* arena) {
    utl_StaticMap* map = arena_alloc(arena, sizeof(utl_StaticMap));
    map->arena_type = 1;
    map->arena.arena_ptr = arena;
    map->buckets_num = buckets_num;
    map->buckets = arena_alloc(arena, sizeof(utl_ListNode*) * buckets_num);
    memset(map->buckets, 0, sizeof(utl_ListNode*) * map->buckets_num);

    return map;
}

void utl_Map_free(utl_StaticMap* map) {
    if(map->arena_type == 0)
        arena_delete(&map->arena.arena);
}

arena_t* arena_ptr(utl_StaticMap* map) {
    return map->arena_type == 0 ? &map->arena.arena : map->arena.arena_ptr;
}

void utl_Map_insert(utl_StaticMap* map, uint32_t key, void* value) {
    utl_ListNodeUint32* current_head = (utl_ListNodeUint32*)map->buckets[key % map->buckets_num];
    utl_ListNodeUint32* new_head = utl_ListNode_append(current_head, key, value, arena_ptr(map));
    if(current_head != new_head) {
        map->buckets[key % map->buckets_num] = (utl_ListNode*)new_head;
    }
}

void utl_Map_remove(utl_StaticMap* map, uint32_t key) {
    utl_ListNodeUint32* current_head = (utl_ListNodeUint32*)map->buckets[key % map->buckets_num];
    utl_ListNodeUint32* new_head = utl_ListNode_remove(current_head, key);
    if(current_head != new_head) {
        map->buckets[key % map->buckets_num] = (utl_ListNode*)new_head;
    }
}

void* utl_Map_search(utl_StaticMap* map, uint32_t key) {
    const utl_ListNodeUint32* head = (utl_ListNodeUint32*)map->buckets[key % map->buckets_num];

    void* found = NULL;
    while(head != NULL) {
        if(head->key == key) {
            found = head->value;
            break;
        }
        head = head->next;
    }

    return found;
}

uint32_t hashStringView(utl_StringView string) {
    uint32_t hash = 7;
    for(int i = 0; i < string.size; i++) {
        hash = hash * 31 + string.data[i];
    }

    return hash;
}

void utl_Map_insert_str(utl_StaticMap* map, utl_StringView key, void* value) {
    uint32_t hash = hashStringView(key);

    utl_ListNodeString* current_head = (utl_ListNodeString*)map->buckets[hash % map->buckets_num];
    utl_ListNodeString* new_head = utl_ListNode_append_str(current_head, utl_StringView_clone(arena_ptr(map), key), value, arena_ptr(map));
    if(current_head != new_head) {
        map->buckets[hash % map->buckets_num] = (utl_ListNode*)new_head;
    }
}

void utl_Map_remove_str(utl_StaticMap* map, utl_StringView key) {
    uint32_t hash = hashStringView(key);

    utl_ListNodeString* current_head = (utl_ListNodeString*)map->buckets[hash % map->buckets_num];
    utl_ListNodeString* new_head = utl_ListNode_remove_str(current_head, key);
    if(current_head != new_head) {
        map->buckets[hash % map->buckets_num] = (utl_ListNode*)new_head;
    }
}

void* utl_Map_search_str(utl_StaticMap* map, utl_StringView key) {
    uint32_t hash = hashStringView(key);

    const utl_ListNodeString* head = (utl_ListNodeString*)map->buckets[hash % map->buckets_num];

    void* found = NULL;
    while(head != NULL) {
        if(head->key.size == key.size && !memcmp(head->key.data, key.data, key.size)) {
            found = head->value;
            break;
        }
        head = head->next;
    }

    return found;
}

void utl_Map_insert_uint64(utl_StaticMap* map, uint64_t key, void* value) {
    utl_ListNodeUint64* current_head = (utl_ListNodeUint64*)map->buckets[key % map->buckets_num];
    utl_ListNodeUint64* new_head = utl_ListNode_append_uint64(current_head, key, value, arena_ptr(map));
    if(current_head != new_head) {
        map->buckets[key % map->buckets_num] = (utl_ListNode*)new_head;
    }
}

void utl_Map_remove_uint64(utl_StaticMap* map, uint64_t key) {
    utl_ListNodeUint64* current_head = (utl_ListNodeUint64*)map->buckets[key % map->buckets_num];
    utl_ListNodeUint64* new_head = utl_ListNode_remove_uint64(current_head, key);
    if(current_head != new_head) {
        map->buckets[key % map->buckets_num] = (utl_ListNode*)new_head;
    }
}

void* utl_Map_search_uint64(utl_StaticMap* map, uint64_t key) {
    const utl_ListNodeUint64* head = (utl_ListNodeUint64*)map->buckets[key % map->buckets_num];

    void* found = NULL;
    while(head != NULL) {
        if(head->key == key) {
            found = head->value;
            break;
        }
        head = head->next;
    }

    return found;
}