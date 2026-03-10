/**
 * @file test_str.c
 * @brief Unit tests for MudStr string utilities
 */

#include "test/test_autoreg.h"
#include "mud_str.h"
#include <string.h>

TEST(strview_create_and_substr) {
    MudStrView sv = mud_strview_from_cstr("Hello");
    CHECK_INT_EQ(ctx, sv.len, 5);
    CHECK(memcmp(sv.data, "Hello", 5) == 0);

    sv = mud_strview_from_cstr(NULL);
    CHECK_INT_EQ(ctx, sv.len, 0);
    CHECK_NULL(ctx, sv.data);

    sv = mud_strview_from_parts("Hello, World!", 5);
    CHECK_INT_EQ(ctx, sv.len, 5);
    CHECK(memcmp(sv.data, "Hello", 5) == 0);

    MudStrView sub = mud_strview_substr(mud_strview_from_cstr("Hello, World!"), 7, 5);
    CHECK_INT_EQ(ctx, sub.len, 5);
    CHECK(memcmp(sub.data, "World", 5) == 0);
}

TEST(strview_compare_prefix_suffix_trim) {
    MudStrView hello = mud_strview_from_cstr("hello");
    MudStrView hello2 = mud_strview_from_cstr("hello");
    MudStrView world = mud_strview_from_cstr("world");

    CHECK(mud_strview_equals(hello, hello2));
    CHECK(!mud_strview_equals(hello, world));
    CHECK(mud_strview_equals_cstr(hello, "hello"));
    CHECK(!mud_strview_equals_cstr(hello, "Hello"));

    MudStrView phrase = mud_strview_from_cstr("Hello, World!");
    CHECK(mud_strview_starts_with(phrase, mud_strview_from_cstr("Hello")));
    CHECK(!mud_strview_starts_with(phrase, mud_strview_from_cstr("hello")));
    CHECK(mud_strview_ends_with(phrase, mud_strview_from_cstr("World!")));
    CHECK(!mud_strview_ends_with(phrase, mud_strview_from_cstr("world!")));

    MudStrView trimmed = mud_strview_trim(mud_strview_from_cstr("  hello \n"));
    CHECK(mud_strview_equals_cstr(trimmed, "hello"));
}

TEST(str_copy_concat_compare) {
    char dest[16];

    size_t copied = mud_str_copy(dest, sizeof(dest), "hello");
    CHECK_INT_EQ(ctx, copied, 5);
    CHECK_STR_EQ(ctx, dest, "hello");

    copied = mud_str_copy(dest, sizeof(dest), "this string is longer");
    CHECK_INT_EQ(ctx, copied, 15);
    CHECK_STR_EQ(ctx, dest, "this string is ");

    mud_str_copy(dest, sizeof(dest), "abc");
    size_t appended = mud_str_concat(dest, sizeof(dest), "def");
    CHECK_INT_EQ(ctx, appended, 3);
    CHECK_STR_EQ(ctx, dest, "abcdef");

    CHECK_INT_EQ(ctx, mud_str_compare("abc", "abc"), 0);
    CHECK(mud_str_compare("abc", "abd") < 0);
    CHECK_INT_EQ(ctx, mud_str_compare_nocase("Hello", "hELLo"), 0);
}

TEST(str_to_numbers_and_bool) {
    int int_value = 0;
    long long_value = 0;
    double double_value = 0.0;
    bool bool_value = false;

    CHECK(mud_str_to_int("42", &int_value));
    CHECK_INT_EQ(ctx, int_value, 42);
    CHECK(!mud_str_to_int("12abc", &int_value));
    CHECK(mud_str_to_int("  17 \t", &int_value));
    CHECK_INT_EQ(ctx, int_value, 17);

    CHECK(mud_str_to_long("-1234567890", &long_value));
    CHECK(long_value == -1234567890L);
    CHECK(!mud_str_to_long("not a number", &long_value));
    CHECK(!mud_str_to_long("44xyz", &long_value));

    CHECK(mud_str_to_double("3.14159", &double_value));
    CHECK(double_value > 3.14 && double_value < 3.15);
    CHECK(mud_str_to_double("  2.5\n", &double_value));
    CHECK(double_value > 2.49 && double_value < 2.51);
    CHECK(!mud_str_to_double("abc", &double_value));

    CHECK(mud_str_to_bool("true", &bool_value));
    CHECK(bool_value);
    CHECK(mud_str_to_bool("OFF", &bool_value));
    CHECK(!bool_value);
    CHECK(mud_str_to_bool(" yes ", &bool_value));
    CHECK(bool_value);
    CHECK(!mud_str_to_bool("maybe", &bool_value));
}

TEST(str_predicates) {
    CHECK(mud_str_is_empty(NULL));
    CHECK(mud_str_is_empty(""));
    CHECK(!mud_str_is_empty("x"));

    CHECK(mud_str_is_blank(NULL));
    CHECK(mud_str_is_blank(" \t\n"));
    CHECK(!mud_str_is_blank(" x "));

    CHECK(mud_str_starts_with("hello world", "hello"));
    CHECK(!mud_str_starts_with("hello world", "world"));
    CHECK(mud_str_ends_with("hello world", "world"));
    CHECK(!mud_str_ends_with("hello world", "hello"));
    CHECK(mud_str_contains("hello world", "o w"));
    CHECK(mud_str_contains("hello", "hello"));
    CHECK(!mud_str_contains("hello", "xyz"));
}
