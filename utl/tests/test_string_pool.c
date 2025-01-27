#include <stdlib.h>
#include <string.h>
#include <unity.h>
#include "string_pool.h"

void setUp() {
}

void tearDown() {
}

void test_alloc_and_free() {
    utl_StringView string = utl_StringPool_alloc(10);

    TEST_ASSERT_EQUAL(10, string.size);
    strncpy(string.data, "0123456789", 10);

    utl_StringPool_free(string);
}

void test_realloc_same_size_type() {
    utl_StringView string = utl_StringPool_alloc(10);

    TEST_ASSERT_EQUAL(10, string.size);
    strncpy(string.data, "0123456789", 10);

    char* old_ptr = string.data;

    string = utl_StringPool_realloc(string, 100);
    TEST_ASSERT_EQUAL(100, string.size);
    TEST_ASSERT_EQUAL(old_ptr, string.data);
    TEST_ASSERT_EQUAL_STRING("0123456789", string.data);

    utl_StringPool_free(string);
}

void test_realloc_bigger_size_type() {
    utl_StringView string = utl_StringPool_alloc(10);

    TEST_ASSERT_EQUAL(10, string.size);
    strncpy(string.data, "0123456789", 10);

    char* old_ptr = string.data;

    // Small -> Medium
    string = utl_StringPool_realloc(string, 1000);
    TEST_ASSERT_EQUAL(1000, string.size);
    TEST_ASSERT_NOT_EQUAL(old_ptr, string.data);
    TEST_ASSERT_EQUAL_STRING("0123456789", string.data);

    // Medium -> Big
    string = utl_StringPool_realloc(string, 2 * 1024 * 1024 - 123);
    TEST_ASSERT_EQUAL(2 * 1024 * 1024 - 123, string.size);
    TEST_ASSERT_NOT_EQUAL(old_ptr, string.data);
    TEST_ASSERT_EQUAL_STRING("0123456789", string.data);

    // Big -> Large
    string = utl_StringPool_realloc(string, 16 * 1024 * 1024 - 123);
    TEST_ASSERT_EQUAL(16 * 1024 * 1024 - 123, string.size);
    TEST_ASSERT_NOT_EQUAL(old_ptr, string.data);
    TEST_ASSERT_EQUAL_STRING("0123456789", string.data);

    utl_StringPool_free(string);
}

void test_alloc_bigger_than_16_mb() {
    utl_StringView string = utl_StringPool_alloc(20 * 1024 * 1024);

    TEST_ASSERT_EQUAL(0, string.size);
    TEST_ASSERT_EQUAL(NULL, string.data);
}


int main() {
    UNITY_BEGIN();

    RUN_TEST(test_alloc_and_free);
    RUN_TEST(test_realloc_same_size_type);
    RUN_TEST(test_realloc_bigger_size_type);
    RUN_TEST(test_alloc_bigger_than_16_mb);

    return UNITY_END();
}
