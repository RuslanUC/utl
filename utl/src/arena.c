#include "arena.h"

#include <stdlib.h>

utl_Arena_page* utl_Arena_alloc_page(const size_t page_size) {
    void* page_data = malloc(sizeof(utl_Arena_page) + page_size);
    utl_Arena_page* page = page_data;
    page->next = NULL;
    page->pos = 0;
    page->data = page_data + sizeof(utl_Arena_page);

    return page;
}

utl_Arena utl_Arena_new(const size_t page_size) {
    utl_Arena_page* head = utl_Arena_alloc_page(page_size);

    const utl_Arena arena = {
        .page_size = page_size,
        .head = head,
        .tail = head,
    };

    return arena;
}

void utl_Arena_free(utl_Arena* arena) {
    arena->page_size = 0;

    utl_Arena_page* head = arena->head;
    while(head != NULL) {
        utl_Arena_page* old_head = head;
        head = head->next;

        free(old_head);
    }
}

void* utl_Arena_alloc(utl_Arena* arena, const size_t n_bytes) {
    if(arena->page_size == 0 || n_bytes > arena->page_size) {
        return NULL;
    }

    if(arena->tail->pos + n_bytes > arena->page_size) {
        if(arena->tail->next) {
            arena->tail->next->pos = 0;
        } else {
            arena->tail->next = utl_Arena_alloc_page(arena->page_size);
        }
        arena->tail = arena->tail->next;
    }

    void* result = arena->tail->data + arena->tail->pos;
    arena->tail->pos += n_bytes;

    return result;
}

void utl_Arena_save(const utl_Arena* arena, utl_Arena_state* state) {
    state->tail = arena->tail;
    state->tail_pos = arena->tail->pos;
}

void utl_Arena_restore(utl_Arena* arena, utl_Arena_state state) {
    arena->tail = state.tail;
    arena->tail->pos = state.tail_pos;
}


