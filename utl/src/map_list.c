#include <memory.h>
#include "map_list.h"

#define UTL_LIST_APPEND_IMPL(SUFFIX, LIST_TYPE, KEY_TYPE, KEY_SET, KEY_CMP) LIST_TYPE* utl_ListNode_append##SUFFIX(LIST_TYPE* head, KEY_TYPE key, void* value, utl_Arena* arena) { \
        if(head == NULL) { \
            LIST_TYPE* result = utl_Arena_alloc(arena, sizeof(LIST_TYPE)); \
            LIST_TYPE* tmp = result;\
            result->base.next = result->base.prev = NULL; \
            KEY_SET; \
            result->base.value = value; \
            return result; \
        } \
        LIST_TYPE* real_head = head; \
        while(head->base.next != NULL) { \
            if(KEY_CMP) { \
                head->base.value = value; \
                return real_head; \
            } \
            head = (LIST_TYPE*)head->base.next; \
        } \
        if(KEY_CMP) { \
            head->base.value = value; \
        } else { \
            LIST_TYPE* new_node = utl_Arena_alloc(arena, sizeof(LIST_TYPE)); \
            LIST_TYPE* tmp = new_node; \
            new_node->base.next = NULL; \
            new_node->base.prev = (utl_ListNode*)head; \
            KEY_SET; \
            new_node->base.value = value; \
            head->base.next = (utl_ListNode*)new_node; \
        } \
        return real_head; \
    }

#define UTL_LIST_REMOVE_IMPL(SUFFIX, LIST_TYPE, KEY_TYPE, KEY_CMP) LIST_TYPE* utl_ListNode_remove##SUFFIX(LIST_TYPE* head, KEY_TYPE key) { \
        if(head == NULL) { \
            return NULL; \
        } \
        LIST_TYPE* element = head; \
        if(KEY_CMP) { \
            return (LIST_TYPE*)head->base.next; \
        } \
        while(element->base.next != NULL) { \
            if(KEY_CMP) { \
                break; \
            } \
            element = (LIST_TYPE*)element->base.next; \
        } \
        if(!(KEY_CMP)) { \
            return head; \
        } \
        element->base.prev->next = element->base.next; \
        element->base.next->prev = element->base.prev; \
        return head; \
    }

UTL_LIST_APPEND_IMPL(, utl_ListNodeUint32, uint32_t, tmp->key = key, head->key == key)
UTL_LIST_REMOVE_IMPL(, utl_ListNodeUint32, uint32_t, element->key == key)

UTL_LIST_APPEND_IMPL(_str, utl_ListNodeString, utl_StringView, tmp->key = utl_StringView_clone(arena, key), head->key.size == key.size && !memcmp(head->key.data, key.data, key.size))
UTL_LIST_REMOVE_IMPL(_str, utl_ListNodeString, utl_StringView, element->key.size == key.size && !memcmp(element->key.data, key.data, key.size))

UTL_LIST_APPEND_IMPL(_uint64, utl_ListNodeUint64, uint64_t, tmp->key = key, head->key == key)
UTL_LIST_REMOVE_IMPL(_uint64, utl_ListNodeUint64, uint64_t, element->key == key)


#undef UTL_LIST_APPEND_IMPL
#undef UTL_LIST_REMOVE_IMPL
