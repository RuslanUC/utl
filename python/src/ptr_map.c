#include "ptr_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

utl_ListNodePtr* utl_ListNode_append_ptr(utl_ListNodePtr* head, void* key, void* value) {
    if (head == NULL) {
        utl_ListNodePtr* new_head = malloc(sizeof(utl_ListNodePtr));
        new_head->base.next = new_head->base.prev = NULL;
        new_head->key = key;
        new_head->base.value = value;
        return new_head;
    }

    utl_ListNodePtr* real_head = head;

    while (head->base.next != NULL) {
        if (head->key == key) {
            head->base.value = value;
            goto ret_real_head;
        }
        head = (utl_ListNodePtr*)head->base.next;
    }
    if (head->key == key) {
        head->base.value = value;
        goto ret_real_head;
    }

    utl_ListNodePtr* new_node = malloc(sizeof(utl_ListNodePtr));
    new_node->base.next = NULL;
    new_node->base.prev = (utl_ListNode*)head;
    new_node->key = key;
    new_node->base.value = value;
    head->base.next = (utl_ListNode*)new_node;

ret_real_head:
    return real_head;
}

utl_ListNodePtr* utl_ListNode_remove_ptr(utl_ListNodePtr* head, const void* key) {
    if(head == NULL) {
        return NULL;
    }

    utl_ListNodePtr* element = head;
    if(head->key == key) {
        utl_ListNodePtr* new_head = (utl_ListNodePtr*)head->base.next;
        free(head);
        return new_head;
    }

    while(element->base.next != NULL) {
        if(element->key == key) {
            break;
        }
        element = (utl_ListNodePtr*)element->base.next;
    }

    if(element->key != key) {
        return head;
    }

    element->base.prev->next = element->base.next;
    if(element->base.next)
        element->base.next->prev = element->base.prev;

    free(element);

    return head;
}

utl_PtrMap* utl_PtrMap_new(const size_t buckets_num) {
    utl_PtrMap* map = malloc(sizeof(utl_PtrMap));
    map->size = 0;
    map->buckets_num = buckets_num;
    map->buckets = malloc(sizeof(utl_ListNodePtr*) * buckets_num);
    memset(map->buckets, 0, sizeof(utl_ListNodePtr*) * map->buckets_num);

    return map;
}

void utl_PtrMap_free(const utl_PtrMap* map) {
    for(size_t i = 0; i < map->buckets_num; i++) {
        utl_ListNode* head = (utl_ListNode*)map->buckets[i];

        while(head != NULL) {
            utl_ListNode* old_head = head;
            head = head->next;
            free(old_head);
        }
    }

    free(map->buckets);
}

void utl_PtrMap_grow(utl_PtrMap* map) {
    const size_t old_buckets_num = map->buckets_num;
    utl_ListNodePtr** old_buckets = map->buckets;
    map->buckets_num *= 1.25;
    map->buckets = malloc(sizeof(utl_ListNodePtr*) * map->buckets_num);
    memset(map->buckets, 0, sizeof(utl_ListNodePtr*) * map->buckets_num);

    for(size_t i = 0; i < old_buckets_num; i++) {
        utl_ListNodePtr* head = old_buckets[i];

        while(head != NULL) {
            utl_PtrMap_insert(map, head->key, head->base.value);
            utl_ListNodePtr* old_head = head;
            head = (utl_ListNodePtr*)head->base.next;
            free(old_head);
        }
    }

    free(old_buckets);
}

void utl_PtrMap_insert(utl_PtrMap* map, void* key, void* value) {
    utl_ListNodePtr* current_head = map->buckets[(size_t)key % map->buckets_num];
    utl_ListNodePtr* new_head = utl_ListNode_append_ptr(current_head, key, value);
    ++map->size;
    if(current_head != new_head) {
        map->buckets[(size_t)key % map->buckets_num] = new_head;
    }

    if(map->size > map->buckets_num * 0.75) {
        utl_PtrMap_grow(map);
    }
}

void utl_PtrMap_remove(utl_PtrMap* map, void* key) {
    utl_ListNodePtr* current_head = map->buckets[(size_t)key % map->buckets_num];
    utl_ListNodePtr* new_head = utl_ListNode_remove_ptr(current_head, key);
    --map->size;
    if(current_head != new_head) {
        map->buckets[(size_t)key % map->buckets_num] = new_head;
    }
}

void* utl_PtrMap_search(const utl_PtrMap* map, void* key) {
    const utl_ListNodePtr* head = map->buckets[(size_t)key % map->buckets_num];

    void* found = NULL;
    while(head != NULL) {
        if(head->key == key) {
            found = head->base.value;
            break;
        }
        head = (utl_ListNodePtr*)head->base.next;
    }

    return found;
}
