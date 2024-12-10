#pragma once

#include <stdint.h>
#include "string_view.h"
#include "arena.h"

typedef struct utl_ListNode {
    struct utl_ListNode* prev;
    struct utl_ListNode* next;
    void* value;
} utl_ListNode;

#define UTL_LIST_STRUCT(TYPE, KEY_TYPE) typedef struct utl_ListNode##TYPE { \
        utl_ListNode base; \
        KEY_TYPE key; \
    } utl_ListNode##TYPE;

UTL_LIST_STRUCT(Uint32, uint32_t)
UTL_LIST_STRUCT(String, utl_StringView)
UTL_LIST_STRUCT(Uint64, uint64_t)

utl_ListNodeUint32* utl_ListNode_append(utl_ListNodeUint32* head, uint32_t key, void* value, utl_Arena* arena);
utl_ListNodeUint32* utl_ListNode_remove(utl_ListNodeUint32* head, uint32_t key);

utl_ListNodeString* utl_ListNode_append_str(utl_ListNodeString* head, utl_StringView key, void* value, utl_Arena* arena);
utl_ListNodeString* utl_ListNode_remove_str(utl_ListNodeString* head, utl_StringView key);

utl_ListNodeUint64* utl_ListNode_append_uint64(utl_ListNodeUint64* head, uint64_t key, void* value, utl_Arena* arena);
utl_ListNodeUint64* utl_ListNode_remove_uint64(utl_ListNodeUint64* head, uint64_t key);