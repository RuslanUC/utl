#include "parser.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

void utl_parse_fieldType(utl_DefPool* def_pool, char* line, const size_t size, utl_FieldDef* field) {
    size_t pos = 0, last = 0;

    utl_FieldType field_type;

    while(pos < size && line[pos] != '<') ++pos;
    if(line[pos] == '<') {
        // probably vector
        utl_MessageDefVector* vector_def = utl_MessageDefVector_new(&def_pool->arena);
        field->type = VECTOR;
        field->sub.vector_def = vector_def;

        ++pos;
        last = pos;

        while(pos < size && line[pos] != '>') ++pos;
        if(line[pos] != '>') {
            // TODO: fail
            return;
        }

        utl_parse_fieldType(def_pool, line + last, pos - last, (utl_FieldDef*)vector_def);
        return;
    }

    utl_TypeDef* sub_type = NULL;

    const size_t type_len = pos - last;
    char* type_str = line + last;
    if(type_len == 1 && !memcmp(type_str, "#", 1)) {
        field_type = FLAGS;
        if(field->name.size > 5) {
            field->flag_info = (field->name.data[5] - '0') << 5;
        } else {
            field->flag_info = 1 << 5;
        }
    } else if(type_len == 3 && !memcmp(type_str, "int", 3)) {
        field_type = INT32;
    } else if(type_len == 4 && !memcmp(type_str, "long", 4)) {
        field_type = INT64;
    } else if(type_len == 6 && !memcmp(type_str, "int128", 6)) {
        field_type = INT128;
    } else if(type_len == 6 && !memcmp(type_str, "int256", 6)) {
        field_type = INT256;
    } else if(type_len == 6 && !memcmp(type_str, "double", 6)) {
        field_type = DOUBLE;
    } else if(type_len == 5 && !memcmp(type_str, "bytes", 5)) {
        field_type = BYTES;
    } else if(type_len == 6 && !memcmp(type_str, "string", 6)) {
        field_type = STRING;
    } else if(type_len == 4 && !memcmp(type_str, "Bool", 4)) {
        field_type = FULL_BOOL;
    } else if(type_len == 4 && !memcmp(type_str, "true", 4)) {
        field_type = BIT_BOOL;
    } else if((type_len == 2 && !memcmp(type_str, "!X", 2)) || type_len == 8 && !memcmp(type_str, "TLObject", 8)) {
        field_type = TLOBJECT;
    } else {
        field_type = TLOBJECT;
        const utl_StringView type_name = {
            .size = pos - last,
            .data = type_str,
        };
        sub_type = utl_DefPool_getType(def_pool, type_name);
        if(!sub_type) {
            // TODO: fail
        }
    }

    field->type = field_type;
    field->sub.type_def = sub_type;
}

utl_MessageDef* utl_parse_line(utl_DefPool* def_pool, char* line, size_t size, utl_Status* status) {
    utl_Arena_state original_state;
    utl_Arena_save(&def_pool->arena, &original_state);

    utl_MessageDef* message_def = utl_MessageDef_new(&def_pool->arena);
    memset(message_def, 0, sizeof(utl_MessageDef));
    int pos = 0, last = 0;

    while(pos < size && line[pos] != '.' && line[pos] != '#') ++pos;

    if(pos == last) {
        // no name/namespace
        if(status) {
            status->ok = false;
            strncpy(status->message, "Expected name or namespace", UTL_STATUS_MAX_MESSAGE_SIZE);
        }
        utl_Arena_restore(&def_pool->arena, original_state);
        return 0;
    }

    if(line[pos] == '.') {
        message_def->namespace_ = utl_StringView_new(&def_pool->arena, pos - last);
        memcpy(message_def->namespace_.data, line, pos);

        last = ++pos;
        while(pos < size && line[pos] != '#') {
            ++pos;
        }

        if(line[pos] != '#') {
            // tl id (after namespace) not found, probably end of string is reached
            if(status) {
                status->ok = false;
                strncpy(status->message, "Expected tl id", UTL_STATUS_MAX_MESSAGE_SIZE);
            }
            utl_Arena_restore(&def_pool->arena, original_state);
            return 0;
        }

        message_def->name = utl_StringView_new(&def_pool->arena, pos - last);
        memcpy(message_def->name.data, line + last, pos - last);
    } else if(line[pos] == '#') {
        message_def->name = utl_StringView_new(&def_pool->arena, pos - last);
        memcpy(message_def->name.data, line + last, pos - last);
    } else {
        // neither namespace nor tl id found, probably end of string is reached
        if(status) {
            status->ok = false;
            strncpy(status->message, "Expected namespace or tl id", UTL_STATUS_MAX_MESSAGE_SIZE);
        }
        utl_Arena_restore(&def_pool->arena, original_state);
        return 0;
    }

    ++pos;

    while(pos < size && line[pos] != ' ') {
        if(line[pos] >= '0' && line[pos] <= '9') {
            message_def->id = (message_def->id << 4) | ((line[pos] - '0') & 0xF);
        } else if(line[pos] >= 'a' && line[pos] <= 'f') {
            message_def->id = (message_def->id << 4) | ((line[pos] - 'a' + 10) & 0xF);
        } else {
            // invalid hex character
            if(status) {
                status->ok = false;
                strncpy(status->message, "Invalid tl id, not hex", UTL_STATUS_MAX_MESSAGE_SIZE);
            }
            utl_Arena_restore(&def_pool->arena, original_state);
            return 0;
        }
        ++pos;
    }

    if(utl_DefPool_hasMessage(def_pool, message_def->id)) {
        utl_Arena_restore(&def_pool->arena, original_state);
        if(status) {
            status->ok = true;
        }
        return utl_DefPool_getMessage(def_pool, message_def->id);
    }

    while(pos < size && line[pos] == ' ') ++pos;

    const size_t fields_start = pos;
    while(pos < size && line[pos] != '=') {
        if(line[pos] == ':')
            message_def->fields_num++;
        ++pos;
    }
    const size_t fields_end = pos - 1;

    if(line[pos] != '=') {
        // type (after "=") expected, but end of string is reached
        if(status) {
            status->ok = false;
            strncpy(status->message, "Expected type", UTL_STATUS_MAX_MESSAGE_SIZE);
        }
        utl_Arena_restore(&def_pool->arena, original_state);
        return 0;
    }
    ++pos;

    while(pos < size && line[pos] == ' ') ++pos;
    last = pos;

    while(pos < size && line[pos] != ';') ++pos;
    if(line[pos] != ';') {
        // ";" expected, but end of string is reached
        if(status) {
            status->ok = false;
            strncpy(status->message, "Expected \";\"", UTL_STATUS_MAX_MESSAGE_SIZE);
        }
        utl_Arena_restore(&def_pool->arena, original_state);
        return 0;
    }

    if(pos == last) {
        // no type
        if(status) {
            status->ok = false;
            strncpy(status->message, "Expected type", UTL_STATUS_MAX_MESSAGE_SIZE);
        }
        utl_Arena_restore(&def_pool->arena, original_state);
        return 0;
    }

    const utl_StringView type_name = utl_StringView_new(&def_pool->arena, pos - last);
    memcpy(type_name.data, line + last, pos - last);
    if(!utl_DefPool_hasType(def_pool, type_name)) {
        utl_TypeDef* type_def = utl_TypeDef_new(&def_pool->arena);
        type_def->name = type_name;
        utl_DefPool_addType(def_pool, type_def);
    }
    message_def->type = utl_DefPool_getType(def_pool, type_name);

    line += fields_start;
    size = fields_end - fields_start;
    pos = 0;

    if(message_def->fields_num)
        message_def->fields = (utl_FieldDef*)utl_Arena_alloc(&def_pool->arena, sizeof(utl_FieldDef) * message_def->fields_num);

    for(int i = 0; i < message_def->fields_num; i++) {
        utl_FieldDef* field = &message_def->fields[i];
        field->num = i;
        field->flag_info = 0;

        while(pos < size && line[pos] == ' ') ++pos;
        last = pos;
        while(pos < size && line[pos] != ':') ++pos;

        field->name = utl_StringView_new(&def_pool->arena, pos - last);
        memcpy(field->name.data, line + last, pos - last);

        ++pos;
        last = pos;

        while(pos < size && line[pos] != ' ' && line[pos] != '?') ++pos;

        if(line[pos] == '?') {
            const uint8_t count = pos - last;
            if(count < 7 || (line[last + 5] != '.' && line[last + 6] != '.')) {
                // "flags.X" expected, but end of string is reached
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Field is optional, but doesn't have valid flagsX.X", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                utl_Arena_restore(&def_pool->arena, original_state);
                return 0;
            }

            uint8_t flag_num = 1;
            uint8_t flag_bit_offset;
            if(line[last + 5] == '.') {
                //  && line[6] >= '0' && line[6] <= '9'
                // flags.X
                flag_bit_offset = 6;
            } else if(line[last + 6] == '.' && line[last + 5] >= '2' && line[last + 5] <= '7') {
                // flagsN.X
                flag_num = line[last + 5] - '0';
                flag_bit_offset = 7;
            } else {
                if(status) {
                    status->ok = false;
                    strncpy(status->message, "Invalid flagsX.X", UTL_STATUS_MAX_MESSAGE_SIZE);
                }
                utl_Arena_restore(&def_pool->arena, original_state);
                return 0;
            }

            uint8_t flag_info = line[last + flag_bit_offset] - '0';
            if(count - flag_bit_offset == 2) {
                flag_info *= 10;
                flag_info += line[last + flag_bit_offset + 1] - '0';
            }

            flag_info |= flag_num << 5;
            field->flag_info = flag_info;

            ++pos;
            last = pos;
            while(pos < size && line[pos] != ' ') ++pos;

            utl_parse_fieldType(def_pool, line + last, pos - last, field);
        } else if(line[pos] == ' ') {
            utl_parse_fieldType(def_pool, line + last, pos - last, field);
        } else {
            // Field type expected, but end of string is reached
            if(status) {
                status->ok = false;
                strncpy(status->message, "Field expected", UTL_STATUS_MAX_MESSAGE_SIZE);
            }
            utl_Arena_restore(&def_pool->arena, original_state);
            return 0;
        }

        if(field->type == FLAGS)
            message_def->flags_num++;
    }

    if(message_def->flags_num) {
        message_def->flags_fields = (utl_FieldDef**)utl_Arena_alloc(&def_pool->arena, sizeof(utl_FieldDef*) * message_def->flags_num);
        size_t flags_i = 0;
        for(int i = 0; i < message_def->fields_num && flags_i < message_def->flags_num; i++) {
            if(message_def->fields[i].type != FLAGS)
                continue;
            message_def->flags_fields[flags_i++] = &message_def->fields[i];
        }
    }

    if(status) {
        status->ok = true;
    }

    utl_DefPool_addMessage(def_pool, message_def);
    return message_def;
}
