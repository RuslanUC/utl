#include "parser.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/posix1_lim.h>

void utl_parse_fieldType(utl_DefPool* def_pool, char* line, size_t size, utl_FieldDef* field, bool is_vector) {
    size_t pos = 0, last = 0;

    utl_FieldType field_type;
    utl_MessageDefBase* sub_message = NULL;

    while(pos < size && line[pos] != '<') ++pos;
    if(line[pos] == '<') {
        // probably vector
        sub_message = (utl_MessageDefBase*)utl_MessageDefVector_new(&def_pool->arena);
        if(is_vector) {
            ((utl_MessageDefVector*)field)->type = VECTOR;
            ((utl_MessageDefVector*)field)->sub_message_def = sub_message;
        } else {
            field->type = VECTOR;
            field->sub_message_def = sub_message;
        }

        ++pos;
        last = pos;

        while(pos < size && line[pos] != '>') ++pos;
        if(line[pos] != '>') {
            // TODO: fail
            return;
        }

        utl_parse_fieldType(def_pool, line + last, pos - last, (utl_FieldDef*)sub_message, true);
        return;
    }

    size_t type_len = pos - last;
    if(type_len == 1 && !memcmp(line + last, "#", 1)) {
        field_type = FLAGS;
        if(field->name.size > 5) {
            field->flag_info = (field->name.data[5] - '0') << 5;
        } else {
            field->flag_info = 1 << 5;
        }
    } else if(type_len == 3 && !memcmp(line + last, "int", 3)) {
        field_type = INT32;
    } else if(type_len == 4 && !memcmp(line + last, "long", 4)) {
        field_type = INT64;
    } else if(type_len == 6 && !memcmp(line + last, "int128", 6)) {
        field_type = INT128;
    } else if(type_len == 6 && !memcmp(line + last, "int256", 6)) {
        field_type = INT256;
    } else if(type_len == 6 && !memcmp(line + last, "double", 6)) {
        field_type = DOUBLE;
    } else if(type_len == 5 && !memcmp(line + last, "bytes", 5)) {
        field_type = BYTES;
    } else if(type_len == 6 && !memcmp(line + last, "string", 6)) {
        field_type = STRING;
    } else if(type_len == 4 && !memcmp(line + last, "Bool", 4)) {
        field_type = FULL_BOOL;
    } else if(type_len == 4 && !memcmp(line + last, "true", 4)) {
        field_type = BIT_BOOL;
    } else if((type_len == 2 && !memcmp(line + last, "!X", 2)) || type_len == 8 && !memcmp(line + last, "TLObject", 8)) {
        field_type = TLOBJECT;
    } else {
        field_type = TLOBJECT;
        utl_StringView type_name = {
            .size = pos - last,
            .data = line + last,
        };
        sub_message = (utl_MessageDefBase*)utl_DefPool_getType(def_pool, type_name);
        if(sub_message) {
            // TODO: fail
        }
    }

    if(is_vector) {
        ((utl_MessageDefVector*)field)->type = field_type;
        ((utl_MessageDefVector*)field)->sub_message_def = sub_message;
    } else {
        field->type = field_type;
        field->sub_message_def = sub_message;
    }
}

utl_MessageDef* utl_parse_line(utl_DefPool* def_pool, char* line, size_t size) {
    size_t original_size = def_pool->arena.size;
    utl_MessageDef* message_def = utl_MessageDef_new(&def_pool->arena);
    memset(message_def, 0, sizeof(utl_MessageDef));
    int pos = 0, last = 0;

    while(pos < size && line[pos] != '.' && line[pos] != '#') ++pos;

    if(pos == last) {
        // no name/namespace
        def_pool->arena.size = original_size;
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
            def_pool->arena.size = original_size;
            return 0;
        }

        message_def->name = utl_StringView_new(&def_pool->arena, pos - last);
        memcpy(message_def->name.data, line + last, pos - last);
    } else if(line[pos] == '#') {
        message_def->name = utl_StringView_new(&def_pool->arena, pos - last);
        memcpy(message_def->name.data, line + last, pos - last);
    } else {
        // neither namespace nor tl id found, probably end of string is reached
        def_pool->arena.size = original_size;
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
            def_pool->arena.size = original_size;
            return 0;
        }
        ++pos;
    }

    if(utl_DefPool_hasMessage(def_pool, message_def->id)) {
        def_pool->arena.size = original_size;
        return utl_DefPool_getMessage(def_pool, message_def->id);
    }

    while(pos < size && line[pos] == ' ') ++pos;

    size_t fields_start = pos;
    while(pos < size && line[pos] != '=') {
        if(line[pos] == ':')
            message_def->fields_num++;
        ++pos;
    }
    size_t fields_end = pos - 1;

    if(line[pos] != '=') {
        // type (after "=") expected, but end of string is reached
        def_pool->arena.size = original_size;
        return 0;
    }
    ++pos;

    while(pos < size && line[pos] == ' ') ++pos;
    last = pos;

    while(pos < size && line[pos] != ';') ++pos;
    if(line[pos] != ';') {
        // ";" expected, but end of string is reached
        def_pool->arena.size = original_size;
        return 0;
    }

    if(pos == last) {
        // no type
        def_pool->arena.size = original_size;
        return 0;
    }

    utl_StringView type_name = utl_StringView_new(&def_pool->arena, pos - last);
    memcpy(type_name.data, line + last, pos - last);
    if(!utl_DefPool_hasType(def_pool, type_name)) {
        utl_TypeDef* type_def = utl_TypeDef_new(&def_pool->arena);
        type_def->name = type_name;
        type_def->message_defs_num = 0;
        type_def->message_defs = NULL;
        utl_DefPool_addType(def_pool, type_def);
    }
    // TODO: add message_def to type (or is message_defs even needed in utl_TypeDef?)
    message_def->type = utl_DefPool_getType(def_pool, type_name);

    //char* real_line = line;
    line += fields_start;
    size = fields_end - fields_start;
    pos = 0;

    if(message_def->fields_num)
        message_def->fields = (utl_FieldDef*)arena_alloc(&def_pool->arena, sizeof(utl_FieldDef) * message_def->fields_num);

    for(int i = 0; i < message_def->fields_num; i++) {
        utl_FieldDef* field = &message_def->fields[i];
        field->num = i;

        while(pos < size && line[pos] == ' ') ++pos;
        last = pos;
        while(pos < size && line[pos] != ':') ++pos;

        field->name = utl_StringView_new(&def_pool->arena, pos - last);
        memcpy(field->name.data, line + last, pos - last);

        ++pos;
        last = pos;

        while(pos < size && line[pos] != ' ' && line[pos] != '?') ++pos;

        if(line[pos] == '?') {
            uint8_t count = pos - last;
            if(count < 7 || (line[last + 5] != '.' && line[last + 6] != '.')) {
                // "flags.X" expected, but end of string is reached
                def_pool->arena.size = original_size;
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
                def_pool->arena.size = original_size;
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

            utl_parse_fieldType(def_pool, line + last, pos - last, field, false);
        } else if(line[pos] == ' ') {
            utl_parse_fieldType(def_pool, line + last, pos - last, field, false);
        } else {
            // Field type expected, but end of string is reached
            def_pool->arena.size = original_size;
            return 0;
        }

        if(field->type == FLAGS)
            message_def->flags_num++;
    }

    if(message_def->flags_num) {
        message_def->flags_fields = (utl_FieldDef*)arena_alloc(&def_pool->arena, sizeof(utl_FieldDef) * message_def->flags_num);
        size_t flags_i = 0;
        for(int i = 0; i < message_def->fields_num && flags_i < message_def->flags_num; i++) {
            if(message_def->fields[i].type != FLAGS)
                continue;
            message_def->flags_fields[flags_i++] = message_def->fields[i];
        }
    }

    utl_DefPool_addMessage(def_pool, message_def);
    return message_def;
}

ssize_t getline_arena(arena_t* arena, FILE* stream) {
    int c;

    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    arena_reset(arena);
    arena->flags |= ARENA_DONTALIGN;

    char* buf;
    while ((c = getc(stream)) != EOF) {
        if (arena->size + 1 >= SSIZE_MAX) {
            errno = EOVERFLOW;
            return -1;
        }
        buf = arena_alloc(arena, 1);
        *buf = (char)c;

        if (c == '\n') {
            break;
        }
    }

    if (ferror(stream) || (feof(stream) && (arena->size == 0))) {
        return -1;
    }

    buf = arena_alloc(arena, 1);
    *buf = '\0';
    return arena->size;
}

void utl_parse_file(utl_DefPool* def_pool, const char* file_name) {
    FILE* fp = fopen(file_name, "r");
    if(!fp)
        return;

    arena_t line_arena = arena_new();

    ssize_t read;
    utl_MessageSection section = TYPES;
    while ((read = getline_arena(&line_arena, fp)) != -1) {
        char* line = line_arena.data;
        if(read < 6 || (line[0] == '/' && line[1] == '/')) {
            continue;
        }
        if(read >= 11 && line[0] == '-' && line[1] == '-' && line[2] == '-') {
            section = !memcmp(line, "---types---", 11) ? TYPES : FUNCTIONS;
            continue;
        }

        utl_MessageDef* message_def = utl_parse_line(def_pool, line, read);
        if(!message_def) {
            printf("Failed to parse: %s\n", line);
            continue;
        }

        message_def->section = section;
    }

    fclose(fp);
    arena_delete(&line_arena);
}
