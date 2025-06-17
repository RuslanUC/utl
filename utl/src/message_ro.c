#include "message_ro.h"

#include <encoder.h>
#include <stdio.h>
#include <stdlib.h>

#include "string.h"
#include "vector.h"
#include "builtins.h"
#include "string_pool.h"
#include "def_pool.h"
#include "decoder.h"
#include "ro.h"

utl_RoMessage* utl_RoMessage_new(utl_MessageDef* message_def, utl_DefPool* def_pool, uint8_t* data, const size_t size) {
    utl_RoMessage* message = malloc(sizeof(utl_RoMessage) + sizeof(ssize_t) * message_def->fields_num);
    message->message_def = message_def;
    message->def_pool = def_pool;
    message->data = data;
    message->size = size;
    message->field_positions = (ssize_t*)(message + 1);

    utl_DecodeBuf buffer = {
        .data = data,
        .pos = 0,
        .size = size,
    };

    if(!utl_RoMessage_get_positions(message_def, def_pool, &buffer, message->field_positions)) {
        free(message);
        return false;
    }

    return message;
}

void utl_RoMessage_free(utl_RoMessage* message) {
    free(message);
}

bool utl_RoMessage_hasField(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->flag_info == 0)
        return true;

    const utl_FieldDef* flags_field = message->message_def->flags_fields[(field->flag_info >> 5) - 1];
    const uint32_t flag = *(uint32_t*)(message->data + message->field_positions[flags_field->num]);
    const uint32_t flag_bit = 1 << (field->flag_info & 0b11111);

    return (flag & flag_bit) == flag_bit;
}

bool utl_RoMessage_equals(const utl_RoMessage* a, const utl_RoMessage* b) {
    if (a == b)
        return true;
    if (a->message_def != b->message_def)
        return false;
    if(a->size != b->size)
        return false;

    return !memcmp(a->data, b->data, a->size);
}

int32_t utl_RoMessage_getInt32(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != INT32 && field->type != FLAGS)
        return 0;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    return *(int32_t*)(message->data + pos);
}

int64_t utl_RoMessage_getInt64(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != INT64)
        return 0;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    return *(int64_t*)(message->data + pos);
}

utl_Int128 utl_RoMessage_getInt128(const utl_RoMessage* message, const utl_FieldDef* field) {
    utl_Int128 result = {0};
    if (field->type != INT128)
        return result;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return result;

    memcpy(result.value, message->data + pos, 16);
    return result;
}

utl_Int256 utl_RoMessage_getInt256(const utl_RoMessage* message, const utl_FieldDef* field) {
    utl_Int256 result = {0};
    if (field->type != INT256)
        return result;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return result;

    memcpy(result.value, message->data + pos, 32);
    return result;
}

double utl_RoMessage_getDouble(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != DOUBLE)
        return 0;

    const ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    return *(double*)(message->data + pos);
}

bool utl_RoMessage_getBool(const utl_RoMessage* message, const utl_FieldDef* field) {
    if(field->type == FULL_BOOL) {
        const ssize_t pos = message->field_positions[field->num];
        if(pos < 0)
            return 0;
        return !memcmp(BOOL_TRUE, message->data + pos, 4);
    } else if (field->type == BIT_BOOL) {
        return utl_RoMessage_hasField(message, field);
    }

    return false;
}

utl_StringView utl_RoMessage_getBytes_internal(const utl_RoMessage* message, const utl_FieldDef* field, const utl_FieldType check_type) {
    const utl_StringView empty = {.size = 0, .data = NULL};

    if (field->type != check_type)
        return empty;

    ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return empty;

    uint32_t count = message->data[pos++];
    if (count >= 254) {
        count = message->data[pos + 0] + (message->data[pos + 1] << 8) + (message->data[pos + 2] << 16);
        pos += 3;
    }

    return (utl_StringView){
        .data = (char*)(message->data + pos),
        .size = count,
    };
}

utl_StringView utl_RoMessage_getBytes(const utl_RoMessage* message, const utl_FieldDef* field) {
    return utl_RoMessage_getBytes_internal(message, field, BYTES);
}

utl_StringView utl_RoMessage_getString(const utl_RoMessage* message, const utl_FieldDef* field) {
    return utl_RoMessage_getBytes_internal(message, field, STRING);
}

utl_RoMessage* utl_RoMessage_getMessage(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != TLOBJECT)
        return 0;

    ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    size_t size;
    if(field->num == message->message_def->fields_num - 1)
        size = message->size - pos;
    else
        size = message->field_positions[field->num + 1] - pos;

    const uint32_t tl_id = *(uint32_t*)(message->data + pos);
    pos += 4;

    utl_MessageDef* new_def = utl_DefPool_getMessage(message->def_pool, tl_id);
    if (!new_def)
        return NULL;

    return utl_RoMessage_new(new_def, message->def_pool, message->data + pos, size - 4);
}

utl_RoVector* utl_RoMessage_getVector(const utl_RoMessage* message, const utl_FieldDef* field) {
    if (field->type != VECTOR) {
        return 0;
    }

    ssize_t pos = message->field_positions[field->num];
    if(pos < 0)
        return 0;

    size_t size;
    if(field->num == message->message_def->fields_num - 1)
        size = message->size - pos;
    else
        size = message->field_positions[field->num + 1] - pos;

    pos += 4;
    const uint32_t elements_count = *(uint32_t*)(message->data + pos);
    pos += 4;

    return utl_RoVector_new(field->sub.vector_def, message->def_pool, message->data + pos, size - 8, elements_count);
}
