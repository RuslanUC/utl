#pragma once

#include <stdint.h>
#include "string_view.h"
#include "arena.h"

typedef struct utl_ListNode {} utl_ListNode;

typedef struct utl_ListNodeUint32 {
    struct utl_ListNodeUint32* prev;
    struct utl_ListNodeUint32* next;
    uint32_t key;
    void* value;
} utl_ListNodeUint32;

typedef struct utl_ListNodeString {
    struct utl_ListNodeString* prev;
    struct utl_ListNodeString* next;
    utl_StringView key;
    void* value;
} utl_ListNodeString;

utl_ListNodeUint32* utl_ListNode_append(utl_ListNodeUint32* head, uint32_t key, void* value, arena_t* arena);
utl_ListNodeUint32* utl_ListNode_remove(utl_ListNodeUint32* head, uint32_t key);

utl_ListNodeString* utl_ListNode_append_str(utl_ListNodeString* head, utl_StringView key, void* value, arena_t* arena);
utl_ListNodeString* utl_ListNode_remove_str(utl_ListNodeString* head, utl_StringView key);
