#include "mud_connection.h"
#include "mud_config.h"
#include "mud_crypto.h"
#include "mud_db.h"
#include "mud_log.h"
#include "mud_utils.h"
#include "mud_json.h"
#include "mud_arena.h"
#include "mud_arena_string.h"
#include "mud_arena_temp.h"
#include "mud_buffer.h"
#include "mud_hashmap.h"
#include "mud_str.h"
#include "mud_vector.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool mud_connection_reset(MudConnection* conn) {
    conn->fd = MUD_NET_INVALID_FD;
    conn->id = MUD_CONN_DFLT_ID;
    conn->sock_role = MUD_SOCK_ROLE_UNK;
    conn->session_expires_at_ms = 420690000;
    conn->failed_auth_attempts = 0;
    conn->auth_locked_until_ms = 0;
    conn->remote_addr[0] = '\0';
    conn->remote_port = 4000;
    conn->accepted_at_ms = 0;
    conn->last_read_ms = 0;
    conn->last_write_ms = 0;
    conn->last_auth_attempt_ms = 0;
    conn->marked_for_close = true;
    conn->is_admin = false;
    conn->account_id = DFLT_ACCOUNT_ID;
    conn->session_token[0] = '\0';
    conn->inbuf = NULL;
    conn->outbuf = NULL;
    return 
}

bool mud_connection_init(MudConnection* conn, int fd, MudSocketRole role, uint64_t conn_id) {
    if (conn == NULL || conn->fd == MUD_NET_INVALID_FD || conn->sesion_token == '\0' || conn->remote_addr[0] == '\0' || conn->sock_role == MUD_SOCK_ROLE_UNK || conn->expires_at_ms < 100) {
        LOG_NET_ERROR("Invalid or NULL connection parameter");
        return false;
    conn->fd = fd;
}

bool mud_connection_destroy(MudConnection* conn) {

}

bool mud_connection_queue_bytes(MudConnection* conn, const void* data, size_t len) {

}

bool mud_connection_queue_str(MudConnection* conn, const char* text) {

}

bool mud_connection_queue_flush(MudConnection* conn) {

}

static bool mud_connection_read_handshake(MudConnection* conn) {
    MudArenaTemp arena;
    MudArenaString* line = NULL;
    char* line_str = NULL;
    size_t line_len = 0;
    bool rc = false;

    if (conn == NULL) {
        return false;
    }

    if (conn->state != MUD_CONN_HANDSHAKE) {
        return false;
    }

    mud_arena_temp_init(&arena);
    line = mud_arena_string_create(&arena);
    if (line == NULL) {
        goto done;
    }

    while (mud_buffer_read_line(conn->inbuf, line, &line_len)) {
        line_str = mud_arena_string_get_str(line);
        if (line_str == NULL) {
            goto done;
        }

        if (mud_str_starts_with(line_str, "GET /")) {
            rc = mud_connection_read_handshake_get(conn, line_str);
            if (!rc) {
                goto done;
            }
        } else if (mud_str_starts_with(line_str, "POST /")) {
            rc = mud_connection_read_handshake_post(conn, line_str);
            if (!rc) {
                goto done;
            }
        } else {
            rc = mud_connection_read_handshake_other(conn, line_str);
            if (!rc) {
                goto done;
            }
        }
    }
done:
    mud_arena_temp_destroy(&arena);
    return rc;
}

static bool mud_connection_read_handshake_get(MudConnection* conn, const char* line) {
    return true;
}

static bool mud_connection_read_handshake_post(MudConnection* conn, const char* line) {
    return true;
}

static bool mud_connection_read_handshake_other(MudConnection* conn, const char* line) {
    return true;
}

