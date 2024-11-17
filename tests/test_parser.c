#include <unity.h>
#include "parser.h"

void setUp() {
}

void tearDown() {
}

void test_MessageWithoutFields() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test#12345678 = Test;", 21);

    TEST_ASSERT_NOT_NULL(message_def);
    TEST_ASSERT_EQUAL_HEX16(0x12345678, message_def->id);
    TEST_ASSERT_EQUAL(0, message_def->fields_num);
    TEST_ASSERT_EQUAL_STRING("test", message_def->name.data);
    TEST_ASSERT_NULL(message_def->namespace_.data);
    TEST_ASSERT_NOT_NULL(message_def->type);
    TEST_ASSERT_EQUAL_STRING("Test", message_def->type->name.data);

    utl_DefPool_free(pool);
}

void test_MessageWithoutId_MustReturnNULL() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test12345678 = Test;", 20);
    TEST_ASSERT_NULL(message_def);

    utl_DefPool_free(pool);
}

void test_MessageIdInvalidHex_MustReturnNULL() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test#g2345678 = Test;", 21);
    TEST_ASSERT_NULL(message_def);

    utl_DefPool_free(pool);
}

void test_MessageWithoutName_MustReturnNULL() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "#12345678 = Test;", 17);
    TEST_ASSERT_NULL(message_def);

    utl_DefPool_free(pool);
}

void test_MessageWithoutEqualsSign_MustReturnNULL() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test#12345678 Test;", 19);
    TEST_ASSERT_NULL(message_def);

    utl_DefPool_free(pool);
}

void test_MessageWithoutType_MustReturnNULL() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test#12345678 = ;", 17);
    TEST_ASSERT_NULL(message_def);

    utl_DefPool_free(pool);
}

void test_MessageWithoutSemicolon_MustReturnNULL() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test#12345678 = Test", 20);
    TEST_ASSERT_NULL(message_def);

    utl_DefPool_free(pool);
}

void test_MessageEmpty_MustReturnNULL() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "", 0);
    TEST_ASSERT_NULL(message_def);

    utl_DefPool_free(pool);
}

void test_MessageWithFields() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test#12345678 a:int b:long c:int128 d:int256 e:double f:string g:bytes h:Bool = Test;", 85);

    TEST_ASSERT_NOT_NULL(message_def);
    TEST_ASSERT_EQUAL_HEX16(0x12345678, message_def->id);
    TEST_ASSERT_EQUAL(8, message_def->fields_num);
    TEST_ASSERT_EQUAL(0, message_def->fields[0].num);
    TEST_ASSERT_EQUAL(INT32, message_def->fields[0].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[0].flag_info);

    TEST_ASSERT_EQUAL(1, message_def->fields[1].num);
    TEST_ASSERT_EQUAL(INT64, message_def->fields[1].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[1].flag_info);

    TEST_ASSERT_EQUAL(2, message_def->fields[2].num);
    TEST_ASSERT_EQUAL(INT128, message_def->fields[2].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[2].flag_info);

    TEST_ASSERT_EQUAL(3, message_def->fields[3].num);
    TEST_ASSERT_EQUAL(INT256, message_def->fields[3].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[3].flag_info);

    TEST_ASSERT_EQUAL(4, message_def->fields[4].num);
    TEST_ASSERT_EQUAL(DOUBLE, message_def->fields[4].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[4].flag_info);

    TEST_ASSERT_EQUAL(5, message_def->fields[5].num);
    TEST_ASSERT_EQUAL(STRING, message_def->fields[5].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[5].flag_info);

    TEST_ASSERT_EQUAL(6, message_def->fields[6].num);
    TEST_ASSERT_EQUAL(BYTES, message_def->fields[6].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[6].flag_info);

    TEST_ASSERT_EQUAL(7, message_def->fields[7].num);
    TEST_ASSERT_EQUAL(FULL_BOOL, message_def->fields[7].type);
    TEST_ASSERT_EQUAL(0, message_def->fields[7].flag_info);

    utl_DefPool_free(pool);
}

void test_MessageWithFlags() {
    utl_DefPool* pool = utl_DefPool_new();

    utl_MessageDef* message_def = utl_parse_line(pool, "test#12345678 flags:# a:flags.0?int b:flags.15?long flags2:# c:flags2.3?int128 h:flags2.4?true = Test;", 102);

    TEST_ASSERT_NOT_NULL(message_def);
    TEST_ASSERT_EQUAL_HEX16(0x12345678, message_def->id);
    TEST_ASSERT_EQUAL(6, message_def->fields_num);

    TEST_ASSERT_EQUAL(0, message_def->fields[0].num);
    TEST_ASSERT_EQUAL(FLAGS, message_def->fields[0].type);
    TEST_ASSERT_EQUAL(0b00100000, message_def->fields[0].flag_info);

    TEST_ASSERT_EQUAL(1, message_def->fields[1].num);
    TEST_ASSERT_EQUAL(INT32, message_def->fields[1].type);
    TEST_ASSERT_EQUAL(0b00100000, message_def->fields[1].flag_info);

    TEST_ASSERT_EQUAL(2, message_def->fields[2].num);
    TEST_ASSERT_EQUAL(INT64, message_def->fields[2].type);
    TEST_ASSERT_EQUAL(0b00101111, message_def->fields[2].flag_info);

    TEST_ASSERT_EQUAL(3, message_def->fields[3].num);
    TEST_ASSERT_EQUAL(FLAGS, message_def->fields[3].type);
    TEST_ASSERT_EQUAL(0b01000000, message_def->fields[3].flag_info);

    TEST_ASSERT_EQUAL(4, message_def->fields[4].num);
    TEST_ASSERT_EQUAL(INT128, message_def->fields[4].type);
    TEST_ASSERT_EQUAL(0b01000011, message_def->fields[4].flag_info);

    TEST_ASSERT_EQUAL(5, message_def->fields[5].num);
    TEST_ASSERT_EQUAL(BIT_BOOL, message_def->fields[5].type);
    TEST_ASSERT_EQUAL(0b01000100, message_def->fields[5].flag_info);

    utl_DefPool_free(pool);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_MessageWithoutFields);
    RUN_TEST(test_MessageWithoutId_MustReturnNULL);
    RUN_TEST(test_MessageIdInvalidHex_MustReturnNULL);
    RUN_TEST(test_MessageWithoutName_MustReturnNULL);
    RUN_TEST(test_MessageWithoutEqualsSign_MustReturnNULL);
    RUN_TEST(test_MessageWithoutType_MustReturnNULL);
    RUN_TEST(test_MessageWithoutSemicolon_MustReturnNULL);
    RUN_TEST(test_MessageEmpty_MustReturnNULL);
    RUN_TEST(test_MessageWithFields);
    RUN_TEST(test_MessageWithFlags);

    return UNITY_END();
}
