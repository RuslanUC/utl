#include <unity.h>
#include "parser.h"
#include "encoder.h"
#include "decoder.h"

void setUp() {
}

void tearDown() {
}

void test_MessageWithoutFields() {
    utl_DefPool* pool = utl_DefPool_new();
    utl_MessageDef* message_def = utl_parse_line(pool, "inputPeerEmpty#7f3b18ea = InputPeer;", 36);
    TEST_ASSERT_NOT_NULL(message_def);

    utl_Message* message = utl_Message_new(message_def);
    arena_t encoder_arena = arena_new();
    encoder_arena.flags |= ARENA_DONTALIGN;
    size_t written_bytes = utl_encode(message, &encoder_arena);
    TEST_ASSERT_EQUAL(4, written_bytes);
    char expected[4] = {0xea, 0x18, 0x3b, 0x7f};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, encoder_arena.data, 4);

    utl_Message_free(message);
    arena_delete(&encoder_arena);
    utl_DefPool_free(pool);
}

void test_EncodeWithArenaAlignment_MustNotEncode() {
    utl_DefPool* pool = utl_DefPool_new();
    utl_MessageDef* message_def = utl_parse_line(pool, "inputPeerEmpty#7f3b18ea = InputPeer;", 36);
    TEST_ASSERT_NOT_NULL(message_def);

    utl_Message* message = utl_Message_new(message_def);
    arena_t encoder_arena = arena_new();
    size_t written_bytes = utl_encode(message, &encoder_arena);
    TEST_ASSERT_EQUAL(0, written_bytes);

    utl_Message_free(message);
    arena_delete(&encoder_arena);
    utl_DefPool_free(pool);
}

void test_MessageSimpleEncode() {
    utl_DefPool* pool = utl_DefPool_new();
    utl_MessageDef* message_def = utl_parse_line(pool, "inputPeerUser#dde8a54c user_id:long access_hash:long = InputPeer;", 65);
    TEST_ASSERT_NOT_NULL(message_def);

    utl_Message* message = utl_Message_new(message_def);
    utl_Message_setInt64(message, &message_def->fields[0], 123456789123);
    utl_Message_setInt64(message, &message_def->fields[1], 987654321321);

    arena_t encoder_arena = arena_new();
    encoder_arena.flags |= ARENA_DONTALIGN;
    size_t written_bytes = utl_encode(message, &encoder_arena);
    TEST_ASSERT_EQUAL(20, written_bytes);
    char expected[20] = {0x4c, 0xa5, 0xe8, 0xdd, 0x83, 0x1a, 0x99, 0xbe, 0x1c, 0x00, 0x00, 0x00, 0xa9, 0xf4, 0xc8, 0xf4, 0xe5, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, encoder_arena.data, 20);

    utl_Message_free(message);
    arena_delete(&encoder_arena);
    utl_DefPool_free(pool);
}

void test_MessageSimpleDecode() {
    utl_DefPool* pool = utl_DefPool_new();
    utl_MessageDef* message_def = utl_parse_line(pool, "inputPeerUser#dde8a54c user_id:long access_hash:long = InputPeer;", 65);
    TEST_ASSERT_NOT_NULL(message_def);

    char bytes[20] = {0x4c, 0xa5, 0xe8, 0xdd, 0x83, 0x1a, 0x99, 0xbe, 0x1c, 0x00, 0x00, 0x00, 0xa9, 0xf4, 0xc8, 0xf4, 0xe5, 0x00, 0x00, 0x00};
    utl_Message* message = utl_Message_new(message_def);
    utl_decode(message, pool, bytes+4, 20);

    TEST_ASSERT_EQUAL(123456789123, utl_Message_getInt64(message, &message_def->fields[0]));
    TEST_ASSERT_EQUAL(987654321321, utl_Message_getInt64(message, &message_def->fields[1]));

    utl_Message_free(message);
    utl_DefPool_free(pool);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_MessageWithoutFields);
    RUN_TEST(test_EncodeWithArenaAlignment_MustNotEncode);
    RUN_TEST(test_MessageSimpleEncode);
    RUN_TEST(test_MessageSimpleDecode);

    return UNITY_END();
}
