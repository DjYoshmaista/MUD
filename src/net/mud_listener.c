#include "mud_listener.h"
#include "mud_connection.h"
#include "mud_net_loop.h"
#include "mud_output.h"
#include "mud_telnet.h"
#include "mud_log.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <uv.h>

static uv_tcp_t g_server;
static bool g_started = false;
static int g_max_connections = 0;

static void free_uv_handle(uv_handle_t* handle) {
    free(handle);
}

static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)handle;
    buf->base = malloc(suggested_size);
    buf->len = buf->base != NULL ? suggested_size : 0;
}

static bool get_peer_name(uv_tcp_t* client, char* out_addr, size_t out_addr_len, int* out_port) {
    struct sockaddr_storage ss;
    int len = (int)sizeof(ss);

    if (out_addr == NULL || out_port == NULL) {
        return false;
    }

    out_addr[0] = '\0';
    *out_port = 0;

    if (uv_tcp_getpeername(client, (struct sockaddr*)&ss, &len) != 0) {
        return false;
    }

    if (ss.ss_family == AF_INET) {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&ss;
        if (uv_ip4_name(addr4, out_addr, out_addr_len) != 0) {
            return false;
        }
        *out_port = ntohs(addr4->sin_port);
        return true;
    }

    if (ss.ss_family == AF_INET6) {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&ss;
        if (uv_ip6_name(addr6, out_addr, out_addr_len) != 0) {
            return false;
        }
        *out_port = ntohs(addr6->sin6_port);
        return true;
    }

    return false;
}

static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    MudConnection* conn = (MudConnection*)stream->data;

    if (nread > 0) {
        mud_connection_touch(conn);
        mud_telnet_recv(conn, buf->base, (size_t)nread);
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            LOG_NET_WARN("Read error on conn=%llu: %s", (unsigned long long)mud_connection_get_id(conn), uv_strerror((int)nread));
        }
        mud_connection_close(conn);
    }

    free(buf->base);
}

static void on_new_connection(uv_stream_t* server, int status) {
    uv_tcp_t* client = NULL;
    MudConnection* conn = NULL;
    char addr[46];
    int port = 0;
    int rc = 0;

    (void)server;

    if (status < 0) {
        LOG_NET_ERROR("Incoming connection failed: %s", uv_strerror(status));
        return;
    }

    client = malloc(sizeof(*client));
    if (client == NULL) {
        LOG_NET_ERROR("Out of memory allocating client handle");
        return;
    }

    rc = uv_tcp_init(mud_net_loop_get(), client);
    if (rc != 0) {
        LOG_NET_ERROR("uv_tcp_init(client) failed: %s", uv_strerror(rc));
        free(client);
        return;
    }

    rc = uv_accept(server, (uv_stream_t*)client);
    if (rc != 0) {
        LOG_NET_ERROR("uv_accept failed: %s", uv_strerror(rc));
        uv_close((uv_handle_t*)client, free_uv_handle);
        return;
    }

    if ((int)mud_connection_table_count() >= g_max_connections) {
        LOG_NET_WARN("Rejecting connection: max_connections=%d reached", g_max_connections);
        uv_close((uv_handle_t*)client, free_uv_handle);
        return;
    }

    if (!get_peer_name(client, addr, sizeof(addr), &port)) {
        snprintf(addr, sizeof(addr), "%s", "unknown");
        port = 0;
    }

    conn = mud_connection_create(client, addr, port);
    if (conn == NULL) {
        uv_close((uv_handle_t*)client, free_uv_handle);
        return;
    }

    if (!mud_telnet_attach(conn)) {
        mud_connection_close(conn);
        return;
    }

    rc = uv_read_start((uv_stream_t*)client, alloc_cb, read_cb);
    if (rc != 0) {
        LOG_NET_ERROR("uv_read_start failed for conn=%llu: %s", (unsigned long long)mud_connection_get_id(conn), uv_strerror(rc));
        mud_connection_close(conn);
        return;
    }

    mud_output_send_line(conn, "Connected to My MUD.");
}

bool mud_listener_start(int port, int max_connections) {
    struct sockaddr_in addr;
    int rc = 0;
    uv_loop_t* loop = NULL;

    if (g_started) {
        return true;
    }

    loop = mud_net_loop_get();
    if (loop == NULL) {
        LOG_NET_ERROR("Event loop is not initialized");
        return false;
    }

    rc = uv_tcp_init(loop, &g_server);
    if (rc != 0) {
        LOG_NET_ERROR("uv_tcp_init(server) failed: %s", uv_strerror(rc));
        return false;
    }

    rc = uv_ip4_addr("0.0.0.0", port, &addr);
    if (rc != 0) {
        LOG_NET_ERROR("uv_ip4_addr failed: %s", uv_strerror(rc));
        return false;
    }

    rc = uv_tcp_bind(&g_server, (const struct sockaddr*)&addr, 0);
    if (rc != 0) {
        LOG_NET_ERROR("uv_tcp_bind failed: %s", uv_strerror(rc));
        return false;
    }

    rc = uv_listen((uv_stream_t*)&g_server, 128, on_new_connection);
    if (rc != 0) {
        LOG_NET_ERROR("uv_listen failed: %s", uv_strerror(rc));
        return false;
    }

    g_started = true;
    g_max_connections = max_connections;
    LOG_NET_INFO("Listening on 0.0.0.0:%d", port);
    return true;
}

void mud_listener_stop(void) {
    if (!g_started) {
        return;
    }

    if (!uv_is_closing((uv_handle_t*)&g_server)) {
        uv_close((uv_handle_t*)&g_server, NULL);
    }

    g_started = false;
    LOG_NET_INFO("Listener stopped");
}
