#include <stdlib.h>
#include <string.h>

#include "vector.h"
#include "message.h"
#include "builtins.h"
#include "string_pool.h"
#include "constants.h"
#include "encoder.h"
#include "logging.h"

utl_Vector* utl_Vector_new(utl_MessageDefVector* vector_def, const int32_t initial_size) {
    utl_Vector* vector = malloc(sizeof(utl_Vector));
    vector->message_def = vector_def;
    vector->userdata = NULL;
    vector->size = 0;
    vector->capacity = initial_size;
    vector->data = calloc(initial_size, vector_def->element_size);
    return vector;
}

utl_StringView utl_Vector_getBytes_internal(const utl_Vector* vector, int32_t index, utl_FieldType check_type);

void utl_Vector_free(utl_Vector* vector) {
    //_UTL_LOG("Freeing tlvector %p", vector);

    if(vector->message_def->type == STRING || vector->message_def->type == BYTES) {
        for(int i = 0; i < vector->size; i++) {
            utl_StringView string = ((utl_StringView*)vector->data)[i];
            if(!string.data)
                continue;
            utl_StringPool_free(string);
        }
    }

    free(vector->data);
    free(vector);
}

int32_t utl_Vector_capacity(const utl_Vector* vector) {
    return vector->capacity;
}

void utl_Vector_resize(utl_Vector* vector, const bool force) {
    const int32_t capacity = utl_Vector_capacity(vector);

    if (force || vector->size >= capacity) {
        const size_t element_size = vector->message_def->element_size;
        const int32_t old_capacity = vector->capacity;
        vector->capacity = capacity * 1.25 + 1;
        vector->data = realloc(vector->data, element_size * vector->capacity);
        memset(vector->data + vector->size * element_size, 0, element_size * (vector->capacity - old_capacity));
    }
}

void utl_Vector_remove(utl_Vector* vector, const int32_t index) {
    if (index >= vector->size) {
        return;
    }

    --vector->size;

    const size_t el_size = vector->message_def->element_size;
    const size_t dst_offset = el_size * index;
    const size_t src_offset = el_size * (index + 1);
    memcpy(vector->data + dst_offset, vector->data + src_offset, (vector->size - index) * el_size);
}

void* utl_Vector_rawValue(const utl_Vector* vector, const int32_t index) {
    if(index >= vector->size) {
        return 0;
    }

    return vector->data + vector->message_def->element_size * index;
}

inline int32_t utl_Vector_size(const utl_Vector* vector) {
    return vector->size;
}

bool utl_Vector_equals(const utl_Vector* a, const utl_Vector* b) {
    if (a == b) {
        return true;
    }
    if (a->size != b->size) {
        return false;
    }
    if (a->message_def == NULL || b->message_def == NULL) {
        return false;
    }
    if (a->message_def->type != b->message_def->type) {
        return false;
    }
    if (a->message_def->type == TLOBJECT && a->message_def->sub.type_def != b->message_def->sub.type_def) {
        return false;
    }

    const size_t el_size = a->message_def->element_size;
    size_t offset = 0;
    for (int32_t i = 0; i < a->size; i++) {
        void* value_a = a->data + offset;
        void* value_b = a->data + offset;

        switch (a->message_def->type) {
            case FLAGS:
            case INT32:
            case INT64:
            case INT128:
            case INT256:
            case DOUBLE:
            case FULL_BOOL:
            case BIT_BOOL: {
                if (memcmp(value_a, value_b, el_size))
                    return false;
                break;
            }
            case BYTES:
            case STRING: {
                if (!utl_StringView_equals(*(utl_StringView*)value_a, *(utl_StringView*)value_b))
                    return false;
                break;
            }
            case TLOBJECT: {
                if (!utl_Message_equals(value_a, value_b))
                    return false;
                break;
            }
            case VECTOR: {
                if (!utl_Vector_equals(value_a, value_b))
                    return false;
                break;
            }

            case STATIC_FIELDS_END:
            case ALL_FIELDS_END:
                return false;
        }

        offset += el_size;
    }

    return true;
}


#define UTL_VECTOR_APPEND(NAME, TL_TYPE, INDEX, C_TYPE) void utl_Vector_append##NAME(utl_Vector* vector, C_TYPE value) { \
        if (vector->message_def->type != TL_TYPE) \
            return; \
        utl_Vector_resize(vector, false); \
        const size_t item_offset = vector->message_def->element_size * INDEX; \
        *(C_TYPE*)(vector->data + item_offset) = value; \
    }

void utl_Vector_appendInt32(utl_Vector* vector, int32_t value) {
    if (vector->message_def->type != INT32) return;
    utl_Vector_resize(vector, 0);
    const size_t item_offset = vector->message_def->element_size * vector->size++;
    *(int32_t*)(vector->data + item_offset) = value;
}

//UTL_VECTOR_APPEND(Int32, INT32, vector->size++, int32_t)
UTL_VECTOR_APPEND(Int64, INT64, vector->size++, int64_t)
UTL_VECTOR_APPEND(Double, DOUBLE, vector->size++, double)
UTL_VECTOR_APPEND(Bool, FULL_BOOL, vector->size++, bool)
UTL_VECTOR_APPEND(Message, TLOBJECT, vector->size++, utl_Message*)
UTL_VECTOR_APPEND(Vector, VECTOR, vector->size++, utl_Vector*)

void utl_Vector_appendInt128(utl_Vector* vector, utl_Int128 value) {
    if (vector->message_def->type != INT128)
        return;

    utl_Vector_resize(vector, false);
    const size_t item_offset = vector->message_def->element_size * vector->size++;
    memcpy(vector->data + item_offset, value.value, 16);
}

void utl_Vector_appendInt256(utl_Vector* vector, utl_Int256 value) {
    if (vector->message_def->type != INT256)
        return;

    utl_Vector_resize(vector, false);
    const size_t item_offset = vector->message_def->element_size * vector->size++;
    memcpy(vector->data + item_offset, value.value, 32);
}

void utl_Vector_setBytes_internal(utl_Vector* vector, const int32_t index, const utl_StringView value) {
    if(value.size > UTL_MAX_STRINT_LENGTH)
        return;

    utl_Vector_resize(vector, false);
    utl_StringView* bytes = vector->data + vector->message_def->element_size * index;

    *bytes = utl_StringPool_realloc(*bytes, value.size);
    memcpy(bytes->data, value.data, value.size);
}

void utl_Vector_appendBytes(utl_Vector* vector, utl_StringView value) {
    if (vector->message_def->type != BYTES)
        return;

    utl_Vector_resize(vector, false);
    utl_Vector_setBytes_internal(vector, vector->size++, value);
}

void utl_Vector_appendString(utl_Vector* vector, utl_StringView value) {
    if (vector->message_def->type != STRING)
        return;

    utl_Vector_resize(vector, false);
    utl_Vector_setBytes_internal(vector, vector->size++, value);
}

#define UTL_VECTOR_SET(NAME, TL_TYPE, C_TYPE) void utl_Vector_set##NAME(const utl_Vector* vector, const int32_t index, C_TYPE value) { \
        if (vector->message_def->type != TL_TYPE || index >= vector->size) \
            return; \
        const size_t item_offset = vector->message_def->element_size * index; \
        *(C_TYPE*)(vector->data + item_offset) = value; \
    }

UTL_VECTOR_SET(Int32, INT32, int32_t)
UTL_VECTOR_SET(Int64, INT64, int64_t)
UTL_VECTOR_SET(Double, DOUBLE, double)
UTL_VECTOR_SET(Message, TLOBJECT, utl_Message*)
UTL_VECTOR_SET(Vector, VECTOR, utl_Vector*)

void utl_Vector_setBool(const utl_Vector* vector, const int32_t index, bool value) {
    static const uint32_t full_bools[2] = {BOOL_FALSE_CONSTR, BOOL_TRUE_CONSTR};
    if(vector->message_def->type != FULL_BOOL || index >= vector->size)
        return;

    const size_t item_offset = vector->message_def->element_size * index;
    *(uint32_t*)(vector->data + item_offset) = full_bools[value != 0];
}

void utl_Vector_setInt128(const utl_Vector* vector, const int32_t index, utl_Int128 value) {
    if (vector->message_def->type != INT128 || index >= vector->size)
        return;

    const size_t item_offset = vector->message_def->element_size * index;
    memcpy(vector->data + item_offset, value.value, 16);
}

void utl_Vector_setInt256(const utl_Vector* vector, const int32_t index, utl_Int256 value) {
    if (vector->message_def->type != INT256 || index >= vector->size)
        return;

    const size_t item_offset = vector->message_def->element_size * index;
    memcpy(vector->data + item_offset, value.value, 32);
}

void utl_Vector_setBytes(utl_Vector* vector, const int32_t index, utl_StringView value) {
    if (vector->message_def->type != BYTES || index >= vector->size)
        return;

    utl_Vector_setBytes_internal(vector, index, value);
}

void utl_Vector_setString(utl_Vector* vector, const int32_t index, utl_StringView value) {
    if (vector->message_def->type != STRING || index >= vector->size)
        return;

    utl_Vector_setBytes_internal(vector, index, value);
}

#define UTL_VECTOR_GET(NAME, TL_TYPE, C_TYPE) C_TYPE utl_Vector_get##NAME(const utl_Vector* vector, const int32_t index) { \
        if (vector->message_def->type != TL_TYPE || index >= vector->size) \
            return 0; \
        const size_t item_offset = vector->message_def->element_size * index; \
        return *(C_TYPE*)(vector->data + item_offset); \
    }

UTL_VECTOR_GET(Int32, INT32, int32_t)
UTL_VECTOR_GET(Int64, INT64, int64_t)
UTL_VECTOR_GET(Double, DOUBLE, double)
UTL_VECTOR_GET(Message, TLOBJECT, utl_Message*)
UTL_VECTOR_GET(Vector, VECTOR, utl_Vector*)

bool utl_Vector_getBool(const utl_Vector* vector, const int32_t index) {
    if(vector->message_def->type != FULL_BOOL || index >= vector->size)
        return 0;
    const size_t item_offset = vector->message_def->element_size * index;

    const uint32_t value = *(uint32_t*)(vector->data + item_offset);
    // True  = 0x997275b5; lower byte is 0b10110101 -> lower two bits are 01 -> &0b10 mask = 00
    // False = 0xbc799737; lower byte is 0b00110111 -> lower two bits are 11 -> &0b10 mask = 10
    return (value & 0b10) == 0;
}

utl_Int128 utl_Vector_getInt128(const utl_Vector* vector, const int32_t index) {
    utl_Int128 result = {0};
    if (vector->message_def->type != INT128 || index >= vector->size) {
        return result;
    }

    memcpy(result.value, vector->data + vector->message_def->element_size * index, 16);
    return result;
}

utl_Int256 utl_Vector_getInt256(const utl_Vector* vector, const int32_t index) {
    utl_Int256 result = {0};
    if (vector->message_def->type != INT256 || index >= vector->size) {
        return result;
    }

    memcpy(result.value, vector->data + vector->message_def->element_size * index, 32);
    return result;
}

utl_StringView utl_Vector_getBytes_internal(const utl_Vector* vector, const int32_t index, const utl_FieldType check_type) {
    const utl_StringView empty = {.size = 0, .data = NULL};
    if (vector->message_def->type != check_type || index >= vector->size)
        return empty;

    const utl_StringView* bytes = vector->data + vector->message_def->element_size * index;
    if (bytes == NULL) {
        return empty;
    }

    return *bytes;
}

utl_StringView utl_Vector_getBytes(const utl_Vector* vector, const int32_t index) {
    return utl_Vector_getBytes_internal(vector, index, BYTES);
}

utl_StringView utl_Vector_getString(const utl_Vector* vector, const int32_t index) {
    return utl_Vector_getBytes_internal(vector, index, STRING);
}
