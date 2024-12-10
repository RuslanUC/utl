#pragma once
#include <stddef.h>

typedef struct utl_Arena_page {
    struct utl_Arena_page* next;
    size_t pos;
    void* data;
} utl_Arena_page;

typedef struct utl_Arena {
    size_t page_size;
    utl_Arena_page* head;
    utl_Arena_page* tail;
} utl_Arena;

typedef struct utl_Arena_state {
    utl_Arena_page* tail;
    size_t tail_pos;
} utl_Arena_state;

utl_Arena utl_Arena_new(size_t page_size);
void utl_Arena_free(utl_Arena* arena);
void* utl_Arena_alloc(utl_Arena* arena, size_t n_bytes);

void utl_Arena_save(const utl_Arena* arena, utl_Arena_state* state);
void utl_Arena_restore(utl_Arena* arena, utl_Arena_state state);
