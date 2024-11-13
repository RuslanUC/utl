#include "encoder.h"

#include <string.h>

void utl_encode_intX(char* value, arena_t* arena, uint8_t bytes_size) {
    char* buf = arena_alloc(arena, bytes_size);
    if(is_big_endian()) {
        for(int i = 0; i < bytes_size; i++) {
            buf[i] = value[bytes_size-i-1];
        }
    } else {
        memcpy(buf, value, bytes_size);
    }
}

void utl_encode_int32(int32_t value, arena_t* arena) {
    utl_encode_intX((char*)&value, arena, 4);
}

void utl_encode_int64(int32_t value, arena_t* arena) {
    utl_encode_intX((char*)&value, arena, 4);
}

void utl_encode_int128(char value[16], arena_t* arena) {
    char* buf = arena_alloc(arena, 16);
    memcpy(buf, value, 16);
}

void utl_encode_int256(char value[32], arena_t* arena) {
    char* buf = arena_alloc(arena, 32);
    memcpy(buf, value, 32);
}

void utl_encode_double(double value, arena_t* arena) {
    utl_encode_int64(*(uint64_t*)&value, arena);
}

void utl_encode_bool(const bool value, arena_t* arena) {
    char* buf = arena_alloc(arena, 4);
    memcpy(buf, value ? BOOL_TRUE : BOOL_FALSE, 4);
}

void utl_encode_bytes(utl_StringView value, arena_t* arena) {
    char* buf;
    size_t total_size = value.size;
    if(value.size >= 254) {
        buf = arena_alloc(arena, 4);
        buf[0] = 254;
        buf[1] = value.size & 0xFF;
        buf[2] = (value.size >> 8) & 0xFF;
        buf[3] = (value.size >> 16) & 0xFF;
    } else {
        buf = arena_alloc(arena, 1);
        buf[0] = value.size;
        ++total_size;
    }

    buf = arena_alloc(arena, value.size);
    memcpy(buf, value.data, value.size);
    uint8_t padding = total_size % 4;
    if(padding) {
        padding = 4 - padding;
        buf = arena_alloc(arena, padding);
        memset(buf, 0, padding);
    }
}

void utl_encode_field(const utl_FieldDef* field, void* value, arena_t* arena) {
    switch (field->type) {
        case INT32: {
            utl_encode_int32(((utl_Int32*)value)->value, arena);
            break;
        }
        case INT64: {
            utl_encode_int64(((utl_Int64*)value)->value, arena);
            break;
        }
        case INT128: {
            utl_encode_int128(((utl_Int128*)value)->value, arena);
            break;
        }
        case INT256: {
            utl_encode_int256(((utl_Int256*)value)->value, arena);
            break;
        }
        case DOUBLE: {
            utl_encode_double(((utl_Double*)value)->value, arena);
            break;
        }
        case BOOL: {
            utl_encode_bool(((utl_Bool*)value)->value, arena);
            break;
        }
        case BYTES:
        case STRING: {
            utl_encode_bytes(((utl_Bytes*)value)->value, arena);
            break;
        }
        case TLOBJECT: {
            utl_encode(value, arena);
            break;
        }
        case VECTOR: {
            // TODO: write vector
            break;
        }
    }
}

size_t utl_encode(const utl_Message* message, arena_t* arena) {
    if((arena->flags & ARENA_DONTALIGN) != ARENA_DONTALIGN) {
        return 0;
    }

    const size_t initial_size = arena->size;

    utl_encode_intX((char*)&message->message_def->id, arena, 4);

    const utl_FieldDef* fields = message->message_def->fields;
    for(int i = 0; i < message->message_def->fields_num; i++) {
        const utl_FieldDef field = fields[i];
        if(message->table[field.num] == NULL) {
            continue;
        }
        void* value = message->table[field.num];
        utl_encode_field(&field, value, arena);
    }

    return arena->size - initial_size;
}