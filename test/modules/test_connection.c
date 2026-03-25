#include "test/test_autoreg.h"

#include "mud_buffer.h"
#include "mud_connection.h"

#include <stdlib.h>

static MudConnection* test_connection_make(const char* addr, int port) {
    uv_tcp_t* handle = calloc(1, sizeof(*handle));
    return mud_connection_create(handle, addr, port);
}

TEST(connection_table_tracks_connections) {
    mud_connection_table_init();
    CHECK_INT_EQ(ctx, mud_connection_table_count(), 0);

    MudConnection* conn = test_connection_make("127.0.0.1", 4000);
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    CHECK_INT_EQ(ctx, mud_connection_table_count(), 1);
    CHECK_PTR_EQ(ctx, mud_connection_table_find(mud_connection_get_id(conn)), conn);
    CHECK_STR_EQ(ctx, mud_connection_get_addr(conn), "127.0.0.1");
    CHECK_INT_EQ(ctx, mud_connection_get_port(conn), 4000);

    mud_connection_destroy(conn);
    CHECK_INT_EQ(ctx, mud_connection_table_count(), 0);
}

TEST(connection_send_queues_without_live_uv_loop) {
    mud_connection_table_init();

    MudConnection* conn = test_connection_make("127.0.0.1", 4001);
    REQUIRE_NOT_NULL(ctx, conn);
    if (ctx->abort_current_test) return;

    const char* send_data = "hello";
    mud_connection_send(conn, send_data, 5);
    CHECK_STR_EQ(ctx, mud_buffer_cstr(conn->output_buf), "hello");
    CHECK_INT_EQ(ctx, mud_buffer_size(conn->output_buf), 5);

    uint64_t before = conn->last_active_at_ms;
    mud_connection_touch(conn);
    CHECK_TRUE(ctx, conn->last_active_at_ms >= before);

    mud_connection_destroy(conn);
}
