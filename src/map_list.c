#include "map_list.h"

utl_ListNode* utl_ListNode_append(utl_ListNode* head, uint32_t key, void* value, arena_t* arena) {
    if(head == NULL) {
        utl_ListNode* result = arena_alloc(arena, sizeof(utl_ListNode));
        result->next = result->prev = NULL;
        result->key = key;
        result->value = value;
        return result;
    }
    utl_ListNode* real_head = head;

    while(head->next != NULL) {
        if(head->key == key) {
            head->value = value;
            return real_head;
        }
        head = head->next;
    }

    if(head->key == key) {
        head->value = value;
    } else {
        utl_ListNode* new_node = arena_alloc(arena, sizeof(utl_ListNode));
        new_node->next = NULL;
        new_node->prev = head;
        new_node->key = key;
        new_node->value = value;
        head->next = new_node;
    }

    return real_head;
}

utl_ListNode* utl_ListNode_remove(utl_ListNode* head, uint32_t key) {
    if(head == NULL) {
        return NULL;
    }
    if(head->key == key) {
        return head->next;
    }
    utl_ListNode* element = head;

    while(element->next != NULL) {
        if(element->key == key) {
            break;
        }
        element = element->next;
    }

    if(element->key != key) {
        return head;
    }

    element->prev->next = element->next;
    element->next->prev = element->prev;

    return head;
}
