#include <memory.h>
#include "map_list.h"

#define UTL_LIST_APPEND_IMPL(SUFFIX, LIST_TYPE, KEY_TYPE, KEY_SET, KEY_CMP) LIST_TYPE* utl_ListNode_append##SUFFIX(LIST_TYPE* head, KEY_TYPE key, void* value, arena_t* arena) { \
        if(head == NULL) { \
            LIST_TYPE* result = arena_alloc(arena, sizeof(LIST_TYPE)); \
            LIST_TYPE* tmp = result;\
            result->next = result->prev = NULL; \
            KEY_SET; \
            result->value = value; \
            return result; \
        } \
        LIST_TYPE* real_head = head; \
        while(head->next != NULL) { \
            if(KEY_CMP) { \
                head->value = value; \
                return real_head; \
            } \
            head = head->next; \
        } \
        if(KEY_CMP) { \
            head->value = value; \
        } else { \
            LIST_TYPE* new_node = arena_alloc(arena, sizeof(LIST_TYPE)); \
            LIST_TYPE* tmp = new_node; \
            new_node->next = NULL; \
            new_node->prev = head; \
            KEY_SET; \
            new_node->value = value; \
            head->next = new_node; \
        } \
        return real_head; \
    }

#define UTL_LIST_REMOVE_IMPL(SUFFIX, LIST_TYPE, KEY_TYPE, KEY_CMP) LIST_TYPE* utl_ListNode_remove##SUFFIX(LIST_TYPE* head, KEY_TYPE key) { \
        if(head == NULL) { \
            return NULL; \
        } \
        LIST_TYPE* element = head; \
        if(KEY_CMP) { \
            return head->next; \
        } \
        while(element->next != NULL) { \
            if(KEY_CMP) { \
                break; \
            } \
            element = element->next; \
        } \
        if(!(KEY_CMP)) { \
            return head; \
        } \
        element->prev->next = element->next; \
        element->next->prev = element->prev; \
        return head; \
    }

UTL_LIST_APPEND_IMPL(, utl_ListNodeUint32, uint32_t, tmp->key = key, head->key == key)
UTL_LIST_REMOVE_IMPL(, utl_ListNodeUint32, uint32_t, element->key == key)

UTL_LIST_APPEND_IMPL(_str, utl_ListNodeString, utl_StringView, tmp->key = utl_StringView_clone(arena, key), head->key.size == key.size && !memcmp(head->key.data, key.data, key.size))
UTL_LIST_REMOVE_IMPL(_str, utl_ListNodeString, utl_StringView, element->key.size == key.size && !memcmp(element->key.data, key.data, key.size))
