#include "vector.h"
#include "message.h"

#include <string.h>
#include "builtins.h"

utl_Vector* utl_Vector_new(utl_MessageDefVector* vector_def, const size_t initial_size) {
    utl_Arena arena = utl_Arena_new(4096);
    utl_Vector* vector = utl_Arena_alloc(&arena, sizeof(utl_Vector));
    vector->message_def = vector_def;
    vector->size = 0;
    vector->capacity = initial_size;
    vector->items = utl_Arena_alloc(&arena, sizeof(void*) * initial_size);
    vector->arena = arena;
    return vector;
}

void utl_Vector_free(utl_Vector* vector) {
    utl_Arena_free(&vector->arena);
}

size_t utl_Vector_capacity(const utl_Vector* vector) {
    return vector->capacity;
}

utl_Arena* utl_Vector_arena(utl_Vector* vector) {
    return &vector->arena;
}

void utl_Vector_resize(utl_Vector* vector, const bool force) {
    const size_t capacity = utl_Vector_capacity(vector);

    if(force || vector->size >= capacity) {
        const void* old_items = vector->items;
        vector->capacity = capacity * 1.25 + 1;
        vector->items = utl_Arena_alloc(&vector->arena, sizeof(void*) * vector->capacity);
        memcpy(vector->items, old_items, sizeof(void*) * vector->size);
    }
}

void utl_Vector_append(utl_Vector* vector, void* element) {
    utl_Vector_resize(vector, false);

    vector->items[vector->size++] = element;
}

void utl_Vector_setValue(const utl_Vector* vector, const size_t index, void* element) {
    if(index >= vector->size) {
        return;
    }

    vector->items[index] = element;
}

void utl_Vector_remove(utl_Vector* vector, const size_t index) {
    if(index >= vector->size) {
        return;
    }

    --vector->size;

    const size_t offset = index * sizeof(void*);
    memcpy(vector->items + offset, vector->items + offset + sizeof(void*), (vector->size - index) * sizeof(void*));
}

void* utl_Vector_value(const utl_Vector* vector, const size_t index) {
    if(index >= vector->size) {
        return 0;
    }

    return vector->items[index];
}

size_t utl_Vector_size(const utl_Vector* vector) {
    return vector->size;
}

bool utl_Vector_equals(const utl_Vector* a, const utl_Vector* b) {
    if(a == b) {
        return true;
    }
    if(a->size != b->size) {
        return false;
    }
    if(a->message_def == NULL || b->message_def == NULL) {
        return false;
    }
    if(a->message_def != NULL && a->message_def->type != b->message_def->type) {
        return false;
    }
    if(a->message_def != NULL && a->message_def->type == TLOBJECT && a->message_def->sub.type_def != b->message_def->sub.type_def) {
        return false;
    }

    for(size_t i = 0; i < a->size; i++) {
        void* value_a = a->items[i];
        void* value_b = a->items[i];

        switch (a->message_def->type) {
            case FLAGS:
            case INT32: {
                if(((utl_Int32*)value_a)->value != ((utl_Int32*)value_b)->value)
                    return false;
                break;
            }
            case INT64: {
                if(((utl_Int64*)value_a)->value != ((utl_Int64*)value_b)->value)
                    return false;
                break;
            }
            case INT128: {
                const char* ia = ((utl_Int128*)value_a)->value;
                const char* ib = ((utl_Int128*)value_b)->value;
                if(memcmp(ia, ib, 16))
                    return false;
                break;
            }
            case INT256: {
                const char* ia = ((utl_Int256*)value_a)->value;
                const char* ib = ((utl_Int256*)value_b)->value;
                if(memcmp(ia, ib, 32))
                    return false;
                break;
            }
            case DOUBLE: {
                if(((utl_Double*)value_a)->value != ((utl_Double*)value_b)->value)
                    return false;
                break;
            }
            case FULL_BOOL:
            case BIT_BOOL: {
                if(((utl_Bool*)value_a)->value != ((utl_Bool*)value_b)->value)
                    return false;
                break;
            }
            case BYTES: {
                if(!utl_StringView_equals(((utl_Bytes*)value_a)->value, ((utl_Bytes*)value_b)->value))
                    return false;
                break;
            }
            case STRING: {
                if(!utl_StringView_equals(((utl_Bytes*)value_a)->value, ((utl_Bytes*)value_b)->value))
                    return false;
                break;
            }
            case TLOBJECT: {
                if(!utl_Message_equals(value_a, value_b))
                    return false;
                break;
            }
            case VECTOR: {
                if(!utl_Vector_equals(value_a, value_b))
                    return false;
                break;
            }
        }
    }

    return true;
}
