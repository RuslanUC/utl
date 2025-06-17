#include "ro.h"
#include "encoder.h"

static bool skip_bytes(utl_DecodeBuf* buffer) {
    const uint8_t* buf = utl_DecodeBuf_read_with_oef_check(buffer, 1);
    if (!buf)
        return false;

    uint32_t count = (uint8_t)buf[0];
    uint8_t offset = 1;
    if (count >= 254) {
        buf = utl_DecodeBuf_read_with_oef_check(buffer, 3);
        if (!buf)
            return false;
        count = (uint8_t)buf[0] + ((uint8_t)buf[1] << 8) + ((uint8_t)buf[2] << 16);
        offset = 0;
    }

    const uint32_t padding = (count + offset) % 4;
    if (!utl_DecodeBuf_read_with_oef_check(buffer, count))
        return false;
    if (padding && !utl_DecodeBuf_read_with_oef_check(buffer, 4 - padding))
        return false;

    return true;
}

static bool skip_tlobject(utl_DecodeBuf* buffer, utl_DefPool* def_pool, utl_TypeDef* type) {
    if (!utl_DecodeBuf_read_with_oef_check(buffer, 4))
        return false;
    buffer->pos -= 4;
    const uint32_t tl_id = utl_decode_int32(buffer);
    utl_MessageDef* new_def = utl_DefPool_getMessage(def_pool, tl_id);
    if (!new_def)
        return false;
    if (type && new_def->type != type)
        return false;
    if(!utl_RoMessage_get_positions(new_def, def_pool, buffer, NULL))
        return false;

    return true;
}

static bool skip_vector(utl_DecodeBuf* buffer, utl_DefPool* def_pool, utl_MessageDefVector* vector_def) {
    if (!utl_DecodeBuf_read_with_oef_check(buffer, 8))
        return false;
    buffer->pos -= 8;
    if (utl_decode_int32(buffer) != VECTOR_CONSTR)
        return false;
    const size_t size = utl_decode_int32(buffer);
    if (!utl_RoVector_get_positions(vector_def, def_pool, buffer, size, NULL))
        return false;

    return true;
}

bool utl_RoVector_get_positions(utl_MessageDefVector* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, size_t elements_count, ssize_t* positions) {
    const size_t buffer_start = buffer->pos;

    for(size_t i = 0; i < elements_count; i++) {
        if(positions)
            positions[i] = buffer->pos - buffer_start;

        switch (def->type) {
            case FLAGS:
            case BIT_BOOL: {
                break;
            }

            case INT32:
            case FULL_BOOL: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 4))
                    return false;
                break;
            }
            case INT64:
            case DOUBLE: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 8))
                    return false;
                break;
            }
            case INT128: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 16))
                    return false;
                break;
            }
            case INT256: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 32))
                    return false;
                break;
            }
            case BYTES:
            case STRING: {
                if(!skip_bytes(buffer))
                    return false;
                break;
            }
            case TLOBJECT: {
                if(!skip_tlobject(buffer, def_pool, def->sub.type_def))
                    return false;
                break;
            }
            case VECTOR: {
                if(!skip_vector(buffer, def_pool, def->sub.vector_def))
                    return false;
                break;
            }

            // Must be unreachable
            case STATIC_FIELDS_END: return false;
        }
    }

    return true;
}

bool utl_RoMessage_get_positions(utl_MessageDef* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, ssize_t* positions) {
    const size_t buffer_start = buffer->pos;
    uint32_t flags_fields[4] = {0};
    size_t flags_num = 0;

    for (int i = 0; i < def->fields_num; i++) {
        const utl_FieldDef field = def->fields[i];

        if (field.flag_info && field.type != FLAGS) {
            const uint8_t flag_bit = field.flag_info & 0b11111;
            const uint32_t flags = flags_fields[(field.flag_info >> 5) - 1];
            const bool field_present = (flags & (1 << flag_bit)) == (1 << flag_bit);
            if (field.type == BIT_BOOL && positions != NULL)
                positions[i] = -1;
            if (!field_present) {
                if (positions != NULL)
                    positions[i] = -1;
                continue;
            }
        }

        if (positions != NULL)
            positions[i] = buffer->pos - buffer_start;

        switch (field.type) {
            case FLAGS:
            case INT32:
            case FULL_BOOL: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 4))
                    return false;
                if (field.type == FLAGS) {
                    buffer->pos -= 4;
                    flags_fields[flags_num++] = utl_decode_int32(buffer);
                }
                break;
            }
            case INT64:
            case DOUBLE: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 8))
                    return false;
                break;
            }
            case INT128: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 16))
                    return false;
                break;
            }
            case INT256: {
                if (!utl_DecodeBuf_read_with_oef_check(buffer, 32))
                    return false;
                break;
            }
            case BIT_BOOL: {
                break;
            }
            case BYTES:
            case STRING: {
                if(!skip_bytes(buffer))
                    return false;
                break;
            }
            case TLOBJECT: {
                if(!skip_tlobject(buffer, def_pool, field.sub.type_def))
                    return false;
                break;
            }
            case VECTOR: {
                if(!skip_vector(buffer, def_pool, field.sub.vector_def))
                    return false;
                break;
            }

            // Must be unreachable
            case STATIC_FIELDS_END: return false;
        }
    }

    return true;
}