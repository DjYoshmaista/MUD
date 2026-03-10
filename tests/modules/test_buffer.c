/**
 * @file test_buffer.c
 * @brief Unit tests for MudBuffer byte buffer implementation
 */

#include "test/test_autoreg.h"
#include "mud_buffer.h"

TEST(buffer_create_destroy) {
    MudBuffer* buf = mud_buffer_create();
    !CHECK_NULL(ctx, buf);
    if (ctx->abort_current_test) return;

    CHECK_INT_EQ(ctx, mud_buffer_size(buf), 0);
    CHECK(mud_buffer_is_empty(buf));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "");
    CHECK(mud_buffer_capacity(buf) >= 128);

    mud_buffer_destroy(buf);
}

TEST(buffer_create_from_and_null_safety) {
    MudBuffer* buf = mud_buffer_create_from("Hello, World!");
    !CHECK_NULL(ctx, buf);
    if (ctx->abort_current_test) return;

    CHECK_INT_EQ(ctx, mud_buffer_size(buf), 13);
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "Hello, World!");
    mud_buffer_destroy(buf);

    buf = mud_buffer_create_from(NULL);
    !CHECK_NULL(ctx, buf);
    if (ctx->abort_current_test) return;
    CHECK(mud_buffer_is_empty(buf));
    mud_buffer_destroy(buf);

    CHECK_INT_EQ(ctx, mud_buffer_size(NULL), 0);
    CHECK_INT_EQ(ctx, mud_buffer_capacity(NULL), 0);
    CHECK(mud_buffer_is_empty(NULL));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(NULL), "");
    CHECK_NULL(ctx, mud_buffer_data(NULL));
    CHECK_INT_EQ(ctx, mud_buffer_char_at(NULL, 0), '\0');
    CHECK(!mud_buffer_append_char(NULL, 'x'));
    CHECK(!mud_buffer_append_str(NULL, "test"));
    CHECK(!mud_buffer_append_bytes(NULL, "data", 4));
    CHECK(!mud_buffer_reserve(NULL, 100));
    mud_buffer_clear(NULL);
    mud_buffer_destroy(NULL);
}

TEST(buffer_append_operations) {
    MudBuffer* buf = mud_buffer_create();
    !CHECK_NULL(ctx, buf);
    if (ctx->abort_current_test) return;

    CHECK(mud_buffer_append_char(buf, 'H'));
    CHECK(mud_buffer_append_char(buf, 'i'));
    CHECK(mud_buffer_append_char(buf, '!'));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "Hi!");

    CHECK(mud_buffer_append_str(buf, " Welcome"));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "Hi! Welcome");

    const char bytes[] = {' ', 'O', 'K'};
    CHECK(mud_buffer_append_bytes(buf, bytes, sizeof(bytes)));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "Hi! Welcome OK");

    CHECK(mud_buffer_append_fmt(buf, " %d", 42));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "Hi! Welcome OK 42");

    mud_buffer_destroy(buf);
}

TEST(buffer_char_access_and_truncate) {
    MudBuffer* buf = mud_buffer_create_from("Hello");
    !CHECK_NULL(ctx, buf);
    if (ctx->abort_current_test) return;

    CHECK_INT_EQ(ctx, mud_buffer_char_at(buf, 0), 'H');
    CHECK_INT_EQ(ctx, mud_buffer_char_at(buf, 4), 'o');
    CHECK_INT_EQ(ctx, mud_buffer_char_at(buf, 10), '\0');

    CHECK(mud_buffer_set_char(buf, 0, 'J'));
    CHECK(mud_buffer_set_char(buf, 4, 'y'));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "Jelly");
    CHECK(!mud_buffer_set_char(buf, 10, 'x'));

    CHECK(mud_buffer_truncate(buf, 3));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(buf), "Jel");
    CHECK_INT_EQ(ctx, mud_buffer_size(buf), 3);

    CHECK(mud_buffer_truncate(buf, 100));
    CHECK_INT_EQ(ctx, mud_buffer_size(buf), 3);

    mud_buffer_destroy(buf);
}

TEST(buffer_clear_clone_equals) {
    MudBuffer* original = mud_buffer_create_from("Original content");
    !CHECK_NULL(ctx, original);
    if (ctx->abort_current_test) return;

    MudBuffer* clone = mud_buffer_clone(original);
    !CHECK_NULL(ctx, clone);
    if (ctx->abort_current_test) { mud_buffer_destroy(original); return; }

    CHECK(mud_buffer_equals(original, clone));
    CHECK(mud_buffer_equals(original, original));
    CHECK(mud_buffer_equals_str(original, "Original content"));

    CHECK(mud_buffer_append_str(clone, " modified"));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(original), "Original content");
    CHECK_STR_EQ(ctx, mud_buffer_cstr(clone), "Original content modified");
    CHECK(!mud_buffer_equals(original, clone));

    mud_buffer_clear(original);
    CHECK(mud_buffer_is_empty(original));
    CHECK_STR_EQ(ctx, mud_buffer_cstr(original), "");

    CHECK(!mud_buffer_equals(original, NULL));
    CHECK(!mud_buffer_equals(NULL, original));
    CHECK(mud_buffer_equals(NULL, NULL));
    CHECK(!mud_buffer_equals_str(original, NULL));

    mud_buffer_destroy(original);
    mud_buffer_destroy(clone);
}

TEST(buffer_reserve_and_growth) {
    MudBuffer* buf = mud_buffer_create();
    !CHECK_NULL(ctx, buf);
    if (ctx->abort_current_test) return;

    CHECK(mud_buffer_reserve(buf, 1000));
    CHECK(mud_buffer_capacity(buf) >= 1000);

    for (int i = 0; i < 1000; i++) {
        CHECK(mud_buffer_append_fmt(buf, "%d,", i));
    }

    CHECK(mud_buffer_size(buf) > 1000);
    CHECK(mud_buffer_cstr(buf)[0] == '0');

    mud_buffer_destroy(buf);
}
