#include <iostream>

extern "C" {
#include "message.h"
#include "def_pool.h"
#include "encoder.h"
#include "decoder.h"
}

void create_message(utl_DefPool* pool) {
    utl_TypeDef* sub_type_def = utl_TypeDef_new(&pool->arena);
    sub_type_def->name = {.size = 4, .data = (char*)"Test123"};
    sub_type_def->message_defs_num = 2;
    sub_type_def->message_defs = (utl_MessageDef**)arena_alloc(&pool->arena, sizeof(void*) * sub_type_def->message_defs_num);

    utl_MessageDef* sub_message_def = utl_MessageDef_new(&pool->arena);
    sub_message_def->id = 456456;
    sub_message_def->name = {.size = 4, .data = (char*)"test1"};
    sub_message_def->namespace_ = {.size = 0, .data = nullptr};
    sub_message_def->type = sub_type_def;
    sub_message_def->section = TYPES;
    sub_message_def->layer = 177;
    sub_message_def->fields_num = 1;
    sub_message_def->fields = (utl_FieldDef*)arena_alloc(&pool->arena, sizeof(utl_FieldDef) * sub_message_def->fields_num);
    sub_message_def->flags_num = 0;
    sub_message_def->flags_fields = nullptr;
    sub_message_def->fields[0].num = 0;
    sub_message_def->fields[0].type = INT32;
    sub_message_def->fields[0].flag_info = 0;
    sub_message_def->fields[0].sub_message_def = nullptr;
    utl_DefPool_add_message(pool, sub_message_def);

    sub_message_def = utl_MessageDef_new(&pool->arena);
    sub_message_def->id = 789789;
    sub_message_def->name = {.size = 4, .data = (char*)"test2"};
    sub_message_def->namespace_ = {.size = 0, .data = nullptr};
    sub_message_def->type = sub_type_def;
    sub_message_def->section = TYPES;
    sub_message_def->layer = 177;
    sub_message_def->fields_num = 1;
    sub_message_def->fields = (utl_FieldDef*)arena_alloc(&pool->arena, sizeof(utl_FieldDef) * sub_message_def->fields_num);
    sub_message_def->flags_num = 0;
    sub_message_def->flags_fields = nullptr;
    sub_message_def->fields[0].num = 0;
    sub_message_def->fields[0].type = INT64;
    sub_message_def->fields[0].flag_info = 0;
    sub_message_def->fields[0].sub_message_def = nullptr;
    utl_DefPool_add_message(pool, sub_message_def);


    utl_TypeDef* type_def = utl_TypeDef_new(&pool->arena);
    type_def->name = {.size = 4, .data = (char*)"Test"};
    type_def->message_defs_num = 1;
    type_def->message_defs = (utl_MessageDef**)arena_alloc(&pool->arena, sizeof(void*) * type_def->message_defs_num);

    utl_MessageDef* message_def = utl_MessageDef_new(&pool->arena);
    message_def->id = 123123;
    message_def->name = {.size = 4, .data = (char*)"test"};
    message_def->namespace_ = {.size = 0, .data = nullptr};
    message_def->type = type_def;
    message_def->section = TYPES;
    message_def->layer = 177;
    message_def->fields_num = 3;
    message_def->fields = (utl_FieldDef*)arena_alloc(&pool->arena, sizeof(utl_FieldDef) * message_def->fields_num);
    message_def->flags_num = 0;
    message_def->flags_fields = nullptr;
    message_def->fields[0].num = 0;
    message_def->fields[0].type = INT32;
    message_def->fields[0].flag_info = 0;
    message_def->fields[0].sub_message_def = nullptr;
    message_def->fields[1].num = 1;
    message_def->fields[1].type = STRING;
    message_def->fields[1].flag_info = 0;
    message_def->fields[1].sub_message_def = nullptr;
    message_def->fields[2].num = 2;
    message_def->fields[2].type = TLOBJECT;
    message_def->fields[2].flag_info = 0;
    message_def->fields[2].sub_message_def = (utl_MessageDefBase*)sub_type_def;

    type_def->message_defs[0] = message_def;

    utl_DefPool_add_message(pool, message_def);
}

int main() {
    auto* pool = utl_DefPool_new();
    create_message(pool);

    utl_MessageDef* message_def = utl_DefPool_get_message(pool, 123123);
    utl_MessageDef* sub_message_def = utl_DefPool_get_message(pool, 789789);

    utl_Message* sub_message = utl_Message_new(sub_message_def);
    utl_Message_setInt64(sub_message, &sub_message_def->fields[0], 123456789123);

    utl_Message* message = utl_Message_new(message_def);
    utl_Message_setInt32(message, &message_def->fields[0], 456456);
    utl_Message_setString(message, &message_def->fields[1], {.size = 3, .data = (char*)"IDK"});
    utl_Message_setMessage(message, &message_def->fields[2], sub_message);
    std::cout << "message.<field 0> = " << utl_Message_getInt32(message, &message_def->fields[0]) << std::endl;
    std::cout << "message.<field 1> = " << utl_Message_getString(message, &message_def->fields[1]).data << std::endl;
    std::cout << "message.<field 2> = " << utl_Message_getInt64(utl_Message_getMessage(message, &message_def->fields[2]), &sub_message_def->fields[0]) << std::endl;

    arena_t encoder_arena = arena_new();
    encoder_arena.flags |= ARENA_DONTALIGN;
    size_t written_bytes = utl_encode(message, &encoder_arena);
    std::cout << "Written bytes: " << written_bytes << "\n";

    utl_Message* decoded_message = utl_Message_new(message_def);
    size_t read_bytes = utl_decode(decoded_message, pool, encoder_arena.data+4, written_bytes); // +4 because, for now, tl id is not used when decoding
    std::cout << "Read bytes: " << read_bytes << "\n";
    std::cout << "decoded_message.<field 0> = " << utl_Message_getInt32(decoded_message, &message_def->fields[0]) << std::endl;
    std::cout << "decoded_message.<field 1> = " << utl_Message_getString(decoded_message, &message_def->fields[1]).data << std::endl;
    std::cout << "decoded_message.<field 2> = " << utl_Message_getInt64(utl_Message_getMessage(decoded_message, &message_def->fields[2]), &sub_message_def->fields[0]) << std::endl;

    arena_delete(&encoder_arena);
    utl_Message_free(decoded_message);
    utl_Message_free(message);
    utl_Message_free(sub_message);
    utl_DefPool_free(pool);

    return 0;
}
