#include "static_map.h"

#include <string.h>
#include <utils.h>

utl_Map* utl_Map_new(size_t buckets_num) {
    arena_t arena = arena_new();
    utl_Map* map = arena_alloc(&arena, sizeof(utl_Map));
    map->arena = arena;
    map->buckets_num = buckets_num;
    map->buckets = arena_alloc(&map->arena, sizeof(utl_ListNode*) * buckets_num);
    memset(map->buckets, 0, sizeof(utl_ListNode*) * buckets_num);

    return map;
}

void utl_Map_free(utl_Map* map) {
    utl_arena_delete(&map->arena);
}

void utl_Map_insert(utl_Map* map, uint32_t key, void* value) {
    utl_ListNode* current_head = map->buckets[key % map->buckets_num];
    utl_ListNode* new_head = utl_ListNode_append(current_head, key, value, &map->arena);
    if(current_head != new_head) {
        map->buckets[key % map->buckets_num] = new_head;
    }
}

void utl_Map_remove(utl_Map* map, uint32_t key) {
    utl_ListNode* current_head = map->buckets[key % map->buckets_num];
    utl_ListNode* new_head = utl_ListNode_remove(current_head, key);
    if(current_head != new_head) {
        map->buckets[key % map->buckets_num] = new_head;
    }
}

void* utl_Map_search(utl_Map* map, uint32_t key) {
    const utl_ListNode* head = map->buckets[key % map->buckets_num];

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
