#pragma once

#include <stdint.h>
#include "arena.h"

typedef struct utl_ListNode {
    struct utl_ListNode* prev;
    struct utl_ListNode* next;
    uint32_t key;
    void* value;
} utl_ListNode;

utl_ListNode* utl_ListNode_append(utl_ListNode* head, uint32_t key, void* value, arena_t* arena);
utl_ListNode* utl_ListNode_remove(utl_ListNode* head, uint32_t key);
