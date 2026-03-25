#include "test/test_autoreg.h"

#include "mud_buffer.h"
#include "mud_connection.h"
#include "mud_output.h"

#include <stdlib.h>

static MudConnection* test_output_make_conn(void) {
    uv_tcp_t* handle = calloc(1, sizeof(*handle));
    return mud_connection_create(handle, "127.0.0.1", 4000);
}

TEST(output_send_line_appends_crlf) {
    mud_connection_table_init();

    MudConnection* conn = test_output_make_conn();
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    mud_output_send_line(conn, "look");
    CHECK_STR_EQ(ctx, mud_buffer_cstr(conn->output_buf), "look\r\n");

    mud_connection_destroy(conn);
}

TEST(output_send_colored_respects_ansi_setting) {
    mud_connection_table_init();

    MudConnection* conn = test_output_make_conn();
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    CHECK_TRUE(ctx, mud_output_supports_color(conn));
    mud_output_send_colored(conn, MUD_ANSI_GREEN, "ok");
    CHECK_STR_EQ(ctx, mud_buffer_cstr(conn->output_buf), "\x1b[32mok\x1b[0m");

    mud_buffer_clear(conn->output_buf);
    conn->ansi_enabled = false;
    CHECK_FALSE(ctx, mud_output_supports_color(conn));
    mud_output_send_colored(conn, MUD_ANSI_GREEN, "plain");
    CHECK_STR_EQ(ctx, mud_buffer_cstr(conn->output_buf), "plain");

    mud_connection_destroy(conn);
}
