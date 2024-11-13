#include <iostream>

extern "C" {
#include "message.h"
#include "encoder.h"
#include "decoder.h"
}

int main() {
    arena_t arena = arena_new();

    utl_TypeDef* type_def = utl_TypeDef_new(&arena);
    type_def->name = {.size = 4, .data = (char*)"Test"};
    type_def->namespace_ = {.size = 0, .data = nullptr};
    type_def->message_defs_num = 1;
    type_def->message_defs = (utl_MessageDef**)arena_alloc(&arena, sizeof(void*) * 1);

    utl_MessageDef* message_def = utl_MessageDef_new(&arena);
    message_def->id = 123123;
    message_def->name = {.size = 4, .data = (char*)"test"};
    message_def->namespace_ = {.size = 0, .data = nullptr};
    message_def->type = type_def;
    message_def->section = TYPES;
    message_def->has_optional = false;
    message_def->layer = 177;
    message_def->fields_num = 2;
    message_def->fields = (utl_FieldDef*)arena_alloc(&arena, sizeof(utl_FieldDef) * message_def->fields_num);
    message_def->fields[0].num = 0;
    message_def->fields[0].type = INT32;
    message_def->fields[0].flag_info = 0;
    message_def->fields[0].sub_message_def = nullptr;
    message_def->fields[1].num = 1;
    message_def->fields[1].type = STRING;
    message_def->fields[1].flag_info = 0;
    message_def->fields[1].sub_message_def = nullptr;

    type_def->message_defs[0] = message_def;

    utl_Message* message = utl_Message_new(message_def);
    utl_Message_setInt32(message, &message_def->fields[0], 456456);
    utl_Message_setString(message, &message_def->fields[1], {.size = 3, .data = (char*)"IDK"});
    std::cout << utl_Message_getInt32(message, &message_def->fields[0]) << std::endl;
    std::cout << utl_Message_getString(message, &message_def->fields[1]).data << std::endl;

    arena_t encoder_arena = arena_new();
    encoder_arena.flags |= ARENA_DONTALIGN;
    size_t written_bytes = utl_encode(message, &encoder_arena);
    std::cout << "Written bytes: " << written_bytes << "\n";

    utl_Message* decoded_message = utl_Message_new(message_def);
    size_t read_bytes = utl_decode(decoded_message, encoder_arena.data+4, written_bytes); // +4 because, for now, tl id is not used when decoding
    std::cout << "Read bytes: " << read_bytes << "\n";
    std::cout << utl_Message_getInt32(decoded_message, &message_def->fields[0]) << std::endl;
    std::cout << utl_Message_getString(decoded_message, &message_def->fields[1]).data << std::endl;

    arena_delete(&encoder_arena);
    utl_Message_free(decoded_message);
    utl_Message_free(message);
    arena_delete(&arena);

    return 0;
}
