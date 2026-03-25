#include "test/test_autoreg.h"
#include "test/test_log.h"

#include "mud_buffer.h"
#include "mud_connection.h"
#include "mud_input.h"

#include <stdlib.h>

static MudConnection* test_input_make_conn(void) {
    uv_tcp_t* handle = calloc(1, sizeof(*handle));
    return mud_connection_create(handle, "127.0.0.1", 4000);
}

static void test_log_bytes(MudTestCtx* ctx, const unsigned char* data, size_t len) {
    size_t i = 0;

    for (i = 0; i < len; i++) {
        TEST_LOG_INFO("byte[%zu] = 0x%02X", i, (unsigned int)data[i]);
    }
}

TEST(input_backspace_0x08_erases_previous_character) {
    static const unsigned char bytes[] = { 'a', 'b', 'c', 0x08, 'd', '\r' };
    mud_connection_table_init();

    MudConnection* conn = test_input_make_conn();
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Feeding bytes for backspace (^H / 0x08)");
    test_log_bytes(ctx, bytes, sizeof(bytes));

    mud_input_recv(conn, (const char*)bytes, sizeof(bytes));

    CHECK_STR_EQ(ctx,
                 mud_buffer_cstr(conn->output_buf),
                 "abc\b \bd\r\nYou said: abd\r\n> ");
    CHECK_TRUE(ctx, mud_buffer_is_empty(conn->input_buf));

    mud_connection_destroy(conn);
}

TEST(input_delete_0x7f_erases_previous_character) {
    static const unsigned char bytes[] = { 'a', 'b', 'c', 0x7f, 'd', '\r' };
    mud_connection_table_init();

    MudConnection* conn = test_input_make_conn();
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Feeding bytes for delete/backspace (^? / 0x7F)");
    test_log_bytes(ctx, bytes, sizeof(bytes));

    mud_input_recv(conn, (const char*)bytes, sizeof(bytes));

    CHECK_STR_EQ(ctx,
                 mud_buffer_cstr(conn->output_buf),
                 "abc\b \bd\r\nYou said: abd\r\n> ");
    CHECK_TRUE(ctx, mud_buffer_is_empty(conn->input_buf));

    mud_connection_destroy(conn);
}

TEST(input_csi_delete_sequence_is_ignored_not_appended) {
    static const unsigned char bytes[] = { 'a', 'b', 0x1b, '[', '3', '~', 'c', '\r' };
    mud_connection_table_init();

    MudConnection* conn = test_input_make_conn();
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Feeding bytes for CSI delete sequence (ESC [ 3 ~)");
    test_log_bytes(ctx, bytes, sizeof(bytes));

    mud_input_recv(conn, (const char*)bytes, sizeof(bytes));

    CHECK_STR_EQ(ctx, mud_buffer_cstr(conn->output_buf), "abc\r\nYou said: abc\r\n> ");
    CHECK_FALSE(ctx, conn->input_in_escape);
    CHECK_INT_EQ(ctx, conn->escape_len, 0);

    mud_connection_destroy(conn);
}

TEST(input_crlf_finishes_only_one_line) {
    static const unsigned char bytes[] = { 'l', 'o', 'o', 'k', '\r', '\n' };
    mud_connection_table_init();

    MudConnection* conn = test_input_make_conn();
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Feeding CRLF line ending");
    test_log_bytes(ctx, bytes, sizeof(bytes));

    mud_input_recv(conn, (const char*)bytes, sizeof(bytes));

    CHECK_STR_EQ(ctx, mud_buffer_cstr(conn->output_buf), "look\r\nYou said: look\r\n> ");
    CHECK_FALSE(ctx, conn->input_saw_cr);

    mud_connection_destroy(conn);
}

TEST(input_tab_is_ignored_but_space_is_preserved) {
    static const unsigned char bytes[] = { 'a', '\t', ' ', 'b', '\r' };
    mud_connection_table_init();

    MudConnection* conn = test_input_make_conn();
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Feeding tab and space");
    test_log_bytes(ctx, bytes, sizeof(bytes));

    mud_input_recv(conn, (const char*)bytes, sizeof(bytes));

    CHECK_STR_EQ(ctx, mud_buffer_cstr(conn->output_buf), "a b\r\nYou said: a b\r\n> ");

    mud_connection_destroy(conn);
}
