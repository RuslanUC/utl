#include "vector_ro.h"

#include <encoder.h>
#include <message_ro.h>
#include <ro.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "string_pool.h"

static size_t static_sizes[STATIC_FIELDS_END] = {
    /* INT32 */ 4,
    /* FLAGS */ 4,
    /* INT64 */ 8,
    /* INT128 */ 16,
    /* INT256 */ 32,
    /* DOUBLE */ 8,
    /* FULL_BOOL */ 4,
    /* BIT_BOOL */ 0,
};

utl_RoVector* utl_RoVector_new(utl_MessageDefVector* message_def, utl_DefPool* def_pool, uint8_t* data, size_t size, size_t elements_count) {
    const bool const_element_size = message_def->type < STATIC_FIELDS_END;

    utl_RoVector* vector = malloc(sizeof(utl_RoVector) + (const_element_size ? 0 : sizeof(size_t) * elements_count));
    vector->message_def = message_def;
    vector->def_pool = def_pool;
    vector->data = data;
    vector->elements_count = elements_count;
    vector->size = size;
    vector->field_positions = const_element_size ? NULL : (ssize_t*)(vector + 1);
    vector->element_size = const_element_size ? static_sizes[message_def->type] : 0;

    utl_DecodeBuf buffer = {
        .data = data,
        .pos = 0,
        .size = size,
    };

    if(!utl_RoVector_get_positions(message_def, def_pool, &buffer, elements_count, vector->field_positions)) {
        free(vector);
        return false;
    }

    return vector;
}

void utl_RoVector_free(utl_RoVector* vector) {
    free(vector->data);
}

void* utl_RoVector_rawValue(const utl_RoVector* vector, const size_t index) {
    if(index >= vector->size) {
        return 0;
    }

    return vector->data + vector->message_def->element_size * index;
}

inline size_t utl_RoVector_size(const utl_RoVector* vector) {
    return vector->elements_count;
}

bool utl_RoVector_equals(const utl_RoVector* a, const utl_RoVector* b) {
    if (a == b)
        return true;
    if (a->size != b->size || a->elements_count != b->elements_count)
        return false;
    if (a->message_def == NULL || b->message_def == NULL)
        return false;
    if (a->message_def != NULL && a->message_def->type != b->message_def->type)
        return false;
    if (a->message_def != NULL && a->message_def->type == TLOBJECT && a->message_def->sub.type_def != b->message_def->sub.type_def)
        return false;

    return memcmp(a->data, b->data, a->size);
}

int32_t utl_RoVector_getInt32(const utl_RoVector* vector, const size_t index) {
    if (vector->message_def->type != INT32 || index >= vector->elements_count)
        return 0;

    const ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return 0;

    return *(int32_t*)(vector->data + pos);
}

int64_t utl_RoVector_getInt64(const utl_RoVector* vector, const size_t index) {
    if (vector->message_def->type != INT64 || index >= vector->elements_count)
        return 0;

    const ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return 0;

    return *(int64_t*)(vector->data + pos);
}

utl_Int128 utl_RoVector_getInt128(const utl_RoVector* vector, const size_t index) {
    utl_Int128 result = {0};
    if (vector->message_def->type != INT128 || index >= vector->elements_count)
        return result;

    const ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return result;

    memcpy(result.value, vector->data + pos, 16);
    return result;
}

utl_Int256 utl_RoVector_getInt256(const utl_RoVector* vector, const size_t index) {
    utl_Int256 result = {0};
    if (vector->message_def->type != INT256 || index >= vector->elements_count)
        return result;

    const ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return result;

    memcpy(result.value, vector->data + pos, 32);
    return result;
}

double utl_RoVector_getDouble(const utl_RoVector* vector, const size_t index) {
    if (vector->message_def->type != DOUBLE || index >= vector->elements_count)
        return 0;

    const ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return 0;

    return *(double*)(vector->data + pos);
}

bool utl_RoVector_getBool(const utl_RoVector* vector, const size_t index) {
    if (vector->message_def->type != FULL_BOOL || index >= vector->elements_count)
        return 0;

    const ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return 0;

    return !memcmp(BOOL_TRUE, vector->data + pos, 4);
}

utl_StringView utl_RoVector_getBytes_internal(const utl_RoVector* vector, const size_t index, const utl_FieldType check_type) {
    const utl_StringView empty = {.size = 0, .data = NULL};

    if (vector->message_def->type != check_type || index >= vector->elements_count)
        return empty;

    ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return empty;

    uint32_t count = vector->data[pos++];
    if (count >= 254) {
        count = vector->data[pos + 0] + (vector->data[pos + 1] << 8) + (vector->data[pos + 2] << 16);
        pos += 3;
    }

    return (utl_StringView){
        .data = (char*)(vector->data + pos),
        .size = count,
    };
}

utl_StringView utl_RoVector_getBytes(const utl_RoVector* vector, const size_t index) {
    return utl_RoVector_getBytes_internal(vector, index, BYTES);
}

utl_StringView utl_RoVector_getString(const utl_RoVector* vector, const size_t index) {
    return utl_RoVector_getBytes_internal(vector, index, STRING);
}

utl_RoMessage* utl_RoVector_getMessage(const utl_RoVector* vector, const size_t index) {
    if (vector->message_def->type != TLOBJECT || index >= vector->elements_count)
        return 0;

    ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return 0;

    size_t size;
    if(index == vector->elements_count - 1)
        size = vector->size - pos;
    else
        size = vector->field_positions[index + 1] - pos;

    const uint32_t tl_id = *(uint32_t*)(vector->data + pos);
    pos += 4;

    utl_MessageDef* new_def = utl_DefPool_getMessage(vector->def_pool, tl_id);
    if (!new_def)
        return NULL;

    return utl_RoMessage_new(new_def, vector->def_pool, vector->data + pos, size - 4);
}

utl_RoVector* utl_RoVector_getVector(const utl_RoVector* vector, const size_t index) {
    if (vector->message_def->type != VECTOR || index >= vector->elements_count)
        return 0;

    ssize_t pos = vector->field_positions ? vector->field_positions[index] : vector->element_size * index;;
    if(pos < 0)
        return 0;

    size_t size;
    if(index == vector->elements_count - 1)
        size = vector->size - pos;
    else
        size = vector->field_positions[index + 1] - pos;

    pos += 4;
    const uint32_t elements_count = *(uint32_t*)(vector->data + pos);
    pos += 4;

    return utl_RoVector_new(vector->message_def->sub.vector_def, vector->def_pool, vector->data + pos, size - 8, elements_count);
}
