#include "mud_connection.h"
#include "mud_log.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct MudWriteReq {
    uv_write_t req;
    MudConnection* conn;
    char* data;
    size_t len;
} MudWriteReq;

static MudConnection* g_connections = NULL;
static size_t g_connection_count = 0;
static uint64_t g_next_id = 1;

static uint64_t now_ms(void) {
    return uv_hrtime() / 1000000ULL;
}

static void flush_output(MudConnection* conn);

static void on_write_done(uv_write_t* req, int status) {
    MudWriteReq* write_req = (MudWriteReq*)req;
    MudConnection* conn = write_req->conn;

    if (status < 0 && conn != NULL) {
        LOG_NET_WARN("Write failed for conn=%llu: %s",
                     (unsigned long long)conn->id,
                     uv_strerror(status));
    }

    free(write_req->data);
    free(write_req);

    if (conn == NULL) {
        return;
    }

    conn->write_pending = false;

    if (status < 0) {
        mud_connection_close(conn);
        return;
    }

    if (mud_buffer_size(conn->output_buf) > 0) {
        flush_output(conn);
    }
}

static void on_connection_closed(uv_handle_t* handle) {
    MudConnection* conn = handle != NULL ? (MudConnection*)handle->data : NULL;
    if (conn != NULL) {
        mud_connection_destroy(conn);
    }
}

static void flush_output(MudConnection* conn) {
    MudWriteReq* write_req = NULL;
    uv_buf_t buf;
    size_t len = 0;
    const char* src = NULL;
    int rc = 0;

    if (conn == NULL || conn->closing || conn->write_pending) {
        return;
    }

    if (conn->handle == NULL || conn->handle->loop == NULL) {
        return;
    }

    len = mud_buffer_size(conn->output_buf);
    if (len == 0) {
        return;
    }

    if (len > UINT_MAX) {
        LOG_NET_ERROR("Output buffer too large for conn=%llu",
                      (unsigned long long)conn->id);
        mud_connection_close(conn);
        return;
    }

    src = mud_buffer_cstr(conn->output_buf);
    write_req = calloc(1, sizeof(*write_req));
    if (write_req == NULL) {
        LOG_NET_ERROR("Out of memory allocating write request");
        mud_connection_close(conn);
        return;
    }

    write_req->data = malloc(len);
    if (write_req->data == NULL) {
        LOG_NET_ERROR("Out of memory allocating write buffer");
        free(write_req);
        mud_connection_close(conn);
        return;
    }

    memcpy(write_req->data, src, len);
    write_req->len = len;
    write_req->conn = conn;
    mud_buffer_clear(conn->output_buf);

    buf = uv_buf_init(write_req->data, (unsigned int)write_req->len);
    conn->write_pending = true;
    rc = uv_write(&write_req->req,
                  (uv_stream_t*)conn->handle,
                  &buf,
                  1,
                  on_write_done);
    if (rc != 0) {
        LOG_NET_ERROR("uv_write failed for conn=%llu: %s",
                      (unsigned long long)conn->id,
                      uv_strerror(rc));
        conn->write_pending = false;
        free(write_req->data);
        free(write_req);
        mud_connection_close(conn);
    }
}

void mud_connection_table_init(void) {
    g_connections = NULL;
    g_connection_count = 0;
    g_next_id = 1;
}

void mud_connection_table_shutdown(void) {
    MudConnection* conn = g_connections;
    while (conn != NULL) {
        MudConnection* next = conn->next_global;
        mud_connection_close(conn);
        conn = next;
    }
}

void mud_connection_table_add(MudConnection* conn) {
    if (conn == NULL) {
        return;
    }

    conn->next_global = g_connections;
    g_connections = conn;
    g_connection_count++;
}

void mud_connection_table_remove(MudConnection* conn) {
    MudConnection** it = &g_connections;

    while (*it != NULL) {
        if (*it == conn) {
            *it = conn->next_global;
            conn->next_global = NULL;
            if (g_connection_count > 0) {
                g_connection_count--;
            }
            return;
        }
        it = &(*it)->next_global;
    }
}

MudConnection* mud_connection_table_find(uint64_t id) {
    MudConnection* it = g_connections;
    while (it != NULL) {
        if (it->id == id) {
            return it;
        }
        it = it->next_global;
    }
    return NULL;
}

size_t mud_connection_table_count(void) {
    return g_connection_count;
}

void mud_connection_table_foreach(void (*fn)(MudConnection*, void*), void* user) {
    MudConnection* it = g_connections;
    while (it != NULL) {
        MudConnection* next = it->next_global;
        fn(it, user);
        it = next;
    }
}

MudConnection* mud_connection_create(uv_tcp_t* handle, const char* addr, int port) {
    MudConnection* conn = calloc(1, sizeof(*conn));
    if (conn == NULL) {
        LOG_NET_ERROR("Failed to allocate MudConnection");
        return NULL;
    }

    conn->handle = handle;
    conn->id = g_next_id++;
    conn->remote_port = port;
    conn->connected_at_ms = now_ms();
    conn->last_active_at_ms = conn->connected_at_ms;
    conn->ansi_enabled = true;
    conn->input_echo_enabled = true;
    conn->term_width = 80;
    conn->term_height = 24;

    if (addr != NULL) {
        snprintf(conn->remote_addr, sizeof(conn->remote_addr), "%s", addr);
    } else {
        snprintf(conn->remote_addr, sizeof(conn->remote_addr), "%s", "unknown");
    }

    conn->input_buf = mud_buffer_create();
    conn->output_buf = mud_buffer_create();
    if (conn->input_buf == NULL || conn->output_buf == NULL) {
        LOG_NET_ERROR("Failed to allocate buffers for conn=%llu",
                      (unsigned long long)conn->id);
        mud_buffer_destroy(conn->input_buf);
        mud_buffer_destroy(conn->output_buf);
        free(conn);
        return NULL;
    }

    if (conn->handle != NULL) {
        conn->handle->data = conn;
    }

    mud_connection_table_add(conn);

    LOG_NET_INFO("Connection %llu accepted from %s:%d",
                 (unsigned long long)conn->id,
                 conn->remote_addr,
                 conn->remote_port);
    return conn;
}

void mud_connection_destroy(MudConnection* conn) {
    if (conn == NULL) {
        return;
    }

    mud_connection_table_remove(conn);

    if (conn->telnet != NULL) {
        telnet_free(conn->telnet);
        conn->telnet = NULL;
    }

    mud_buffer_destroy(conn->input_buf);
    mud_buffer_destroy(conn->output_buf);
    conn->input_buf = NULL;
    conn->output_buf = NULL;

    free(conn->handle);
    conn->handle = NULL;
    free(conn);
}

void mud_connection_send(MudConnection* conn, const char* data, size_t len) {
    if (conn == NULL || conn->closing || data == NULL || len == 0) {
        return;
    }

    if (!mud_buffer_append_bytes(conn->output_buf, data, len)) {
        LOG_NET_ERROR("Failed to queue output for conn=%llu",
                      (unsigned long long)conn->id);
        mud_connection_close(conn);
        return;
    }

    flush_output(conn);
}

void mud_connection_send_unsigned(MudConnection* conn, const unsigned char* data, size_t len) {
    if (conn == NULL || conn->closing || data == NULL || len == 0) {
        return;
    }

    if (!mud_buffer_append_bytes(conn->output_buf, data, len)) {
        LOG_NET_ERROR("Failed to queue output for conn=%llu",
                      (unsigned long long)conn->id);
        mud_connection_close(conn);
        return;
    }

    flush_output(conn);
}

void mud_connection_close(MudConnection* conn) {
    if (conn == NULL || conn->closing) {
        return;
    }

    conn->closing = true;

    if (conn->handle == NULL || conn->handle->loop == NULL) {
        mud_connection_destroy(conn);
        return;
    }

    uv_read_stop((uv_stream_t*)conn->handle);

    if (!uv_is_closing((uv_handle_t*)conn->handle)) {
        uv_close((uv_handle_t*)conn->handle, on_connection_closed);
    }

    LOG_NET_INFO("Closing connection %llu from %s:%d",
                 (unsigned long long)conn->id,
                 conn->remote_addr,
                 conn->remote_port);
}

uint64_t mud_connection_get_id(const MudConnection* conn) {
    return conn != NULL ? conn->id : 0;
}

const char* mud_connection_get_addr(const MudConnection* conn) {
    return conn != NULL ? conn->remote_addr : "";
}

int mud_connection_get_port(const MudConnection* conn) {
    return conn != NULL ? conn->remote_port : 0;
}

void mud_connection_touch(MudConnection* conn) {
    if (conn != NULL) {
        conn->last_active_at_ms = now_ms();
    }
}
