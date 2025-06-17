#include <stdlib.h>

#include "string_pool.h"

#include <stdio.h>
#include <string.h>

#include "string_view.h"

#define UTL_STRING_POOL_STRUCT_NONDATA_SIZE (sizeof(size_t) + sizeof(void*) + sizeof(void*))
#define UTL_STRING_POOL_OBJECTS_PER_ALLOC 8

#define UTL_STRING_POOL_SMALL_SIZE 256
#define UTL_STRING_POOL_SMALL_SIZE_MAX UTL_STRING_POOL_SMALL_SIZE - UTL_STRING_POOL_STRUCT_NONDATA_SIZE

#define UTL_STRING_POOL_MEDIUM_SIZE 64 * 1024
#define UTL_STRING_POOL_MEDIUM_SIZE_MAX UTL_STRING_POOL_MEDIUM_SIZE - UTL_STRING_POOL_STRUCT_NONDATA_SIZE

#define UTL_STRING_POOL_BIG_SIZE 2 * 1024 * 1024
#define UTL_STRING_POOL_BIG_SIZE_MAX UTL_STRING_POOL_BIG_SIZE - UTL_STRING_POOL_STRUCT_NONDATA_SIZE

#define UTL_STRING_POOL_LARGE_SIZE 16 * 1024 * 1024
#define UTL_STRING_POOL_LARGE_SIZE_MAX UTL_STRING_POOL_LARGE_SIZE - UTL_STRING_POOL_STRUCT_NONDATA_SIZE

#define UTL_STRING_POOL_STRUCTS(NAME, DATA_SIZE) struct utl_StringPoolPage##NAME; \
    typedef struct utl_StringPoolObject##NAME { \
        struct utl_StringPoolPage##NAME* page; \
        struct utl_StringPoolObject##NAME* next; \
        size_t length; \
        char data[DATA_SIZE]; \
    } utl_StringPoolObject##NAME; \
    typedef struct utl_StringPoolPage##NAME { \
        size_t max_size; \
        size_t free_objects; \
        struct utl_StringPoolPage##NAME* next; \
        utl_StringPoolObject##NAME* objects; \
    } utl_StringPoolPage##NAME;

UTL_STRING_POOL_STRUCTS(Base, 0)

UTL_STRING_POOL_STRUCTS(Small, UTL_STRING_POOL_SMALL_SIZE_MAX)
utl_StringPoolPageSmall* small_current_page = NULL;
utl_StringPoolObjectSmall* small_free_list = NULL;

UTL_STRING_POOL_STRUCTS(Medium, UTL_STRING_POOL_MEDIUM_SIZE_MAX)
utl_StringPoolPageMedium* medium_current_page = NULL;
utl_StringPoolObjectMedium* medium_free_list = NULL;

UTL_STRING_POOL_STRUCTS(Big, UTL_STRING_POOL_BIG_SIZE_MAX)
utl_StringPoolPageBig* big_current_page = NULL;
utl_StringPoolObjectBig* big_free_list = NULL;

UTL_STRING_POOL_STRUCTS(Large, UTL_STRING_POOL_LARGE_SIZE_MAX)
utl_StringPoolPageLarge* large_current_page = NULL;
utl_StringPoolObjectLarge* large_free_list = NULL;

static inline void utl_StringPool_maybe_alloc_new_page(utl_StringPoolPageBase** current_page_p, utl_StringPoolObjectBase** free_list_p, size_t object_size) {
    utl_StringPoolPageBase* current_page = *current_page_p;
    utl_StringPoolObjectBase* free_list = *free_list_p;

    if (free_list != NULL)
        return;

    utl_StringPoolPageBase* next_page = current_page;
    current_page = malloc(sizeof(utl_StringPoolPageBase) + object_size * UTL_STRING_POOL_OBJECTS_PER_ALLOC);
    current_page->max_size = object_size - UTL_STRING_POOL_STRUCT_NONDATA_SIZE;
    current_page->free_objects = UTL_STRING_POOL_OBJECTS_PER_ALLOC;
    current_page->next = next_page;
    current_page->objects = (utl_StringPoolObjectBase*)((char*)current_page + sizeof(utl_StringPoolPageBase));

    for (size_t i = 0; i < UTL_STRING_POOL_OBJECTS_PER_ALLOC; ++i) {
        utl_StringPoolObjectBase* obj = (utl_StringPoolObjectBase*)((char*)current_page->objects + i * object_size);
        obj->page = current_page;
        obj->next = free_list;
        free_list = obj;
    }

    *current_page_p = current_page;
    *free_list_p = free_list;
}

utl_StringView utl_StringPool_alloc(size_t size) {
    ++size; // Because we need to add extra \0 to the end of string

    utl_StringPoolObjectBase* string = NULL;
    if(size <= UTL_STRING_POOL_SMALL_SIZE_MAX) {
        utl_StringPool_maybe_alloc_new_page(
            (utl_StringPoolPageBase**)&small_current_page,
            (utl_StringPoolObjectBase**)&small_free_list,
            sizeof(utl_StringPoolObjectSmall));

        string = (utl_StringPoolObjectBase*)small_free_list;
        small_free_list = small_free_list->next;
    } else if(size <= UTL_STRING_POOL_MEDIUM_SIZE_MAX) {
        utl_StringPool_maybe_alloc_new_page(
            (utl_StringPoolPageBase**)&medium_current_page,
            (utl_StringPoolObjectBase**)&medium_free_list,
            sizeof(utl_StringPoolObjectMedium));

        string = (utl_StringPoolObjectBase*)medium_free_list;
        medium_free_list = medium_free_list->next;
    } else if(size <= UTL_STRING_POOL_BIG_SIZE_MAX) {
        utl_StringPool_maybe_alloc_new_page(
            (utl_StringPoolPageBase**)&big_current_page,
            (utl_StringPoolObjectBase**)&big_free_list,
            sizeof(utl_StringPoolObjectBig));

        string = (utl_StringPoolObjectBase*)big_free_list;
        big_free_list = big_free_list->next;
    } else if(size <= UTL_STRING_POOL_LARGE_SIZE_MAX) {
        utl_StringPool_maybe_alloc_new_page(
            (utl_StringPoolPageBase**)&large_current_page,
            (utl_StringPoolObjectBase**)&large_free_list,
            sizeof(utl_StringPoolObjectLarge));

        string = (utl_StringPoolObjectBase*)large_free_list;
        large_free_list = large_free_list->next;
    } else {
        return (utl_StringView){
            .size = 0,
            .data = NULL,
        };
    }
    --string->page->free_objects;
    string->length = size;

    --size;

    const utl_StringView ret = {
        .size = size,
        .data = string->data,
    };

    ret.data[size] = '\0';
    return ret;
}

void utl_StringPool_free(utl_StringView string) {
    utl_StringPoolObjectBase* obj = (utl_StringPoolObjectBase*)(string.data - UTL_STRING_POOL_STRUCT_NONDATA_SIZE);
    ++obj->page->free_objects;

    utl_StringPoolObjectBase* free_list = NULL;
    switch (obj->page->max_size + UTL_STRING_POOL_STRUCT_NONDATA_SIZE) {
        case sizeof(utl_StringPoolObjectSmall): {
            free_list = (utl_StringPoolObjectBase*)small_free_list;
            small_free_list = (utl_StringPoolObjectSmall*)obj;
            break;
        }
        case sizeof(utl_StringPoolObjectMedium): {
            free_list = (utl_StringPoolObjectBase*)medium_free_list;
            medium_free_list = (utl_StringPoolObjectMedium*)obj;
            break;
        }
        case sizeof(utl_StringPoolObjectBig): {
            free_list = (utl_StringPoolObjectBase*)big_free_list;
            big_free_list = (utl_StringPoolObjectBig*)obj;
            break;
        }
        case sizeof(utl_StringPoolObjectLarge): {
            free_list = (utl_StringPoolObjectBase*)large_free_list;
            large_free_list = (utl_StringPoolObjectLarge*)obj;
            break;
        }
        default: {
            printf("UNREACHABLE\n");
            return;
        }
    }

    obj->next = free_list;
}

utl_StringView utl_StringPool_realloc(utl_StringView string, size_t size) {
    if(string.data == NULL) {
        return utl_StringPool_alloc(size);
    }

    const utl_StringPoolObjectSmall* obj = (utl_StringPoolObjectSmall*)(string.data - UTL_STRING_POOL_STRUCT_NONDATA_SIZE);
    if(size + 1 > obj->page->max_size) {
        utl_StringView new_string = utl_StringPool_alloc(size);
        memcpy(new_string.data, string.data, string.size);
        utl_StringPool_free(string);
        return new_string;
    }

    string.size = size;
    string.data[size] = '\0';
    return string;
}
