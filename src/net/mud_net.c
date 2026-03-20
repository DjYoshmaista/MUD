#include "mud_net.h"
#include "mud_log.h"
#include "mud_buffer.h"
#include "mud_connection.h"
#include "mud_socket.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

static MudConnection* mud_net_alloc_conn_slot(MudNet* net) {
    MudConnection* conn = NULL;
    for (size_t i = 0; i < MUD_NET_MAX_CONNECTIONS; i++) {
        if (net->conns[i].fd == MUD_NET_INVALID_FD) {
            conn = &net->conns[i];
            break;
        }
    }
    return conn;
}

bool mud_net_init_from_config(MudNet* net) {
    if (net == NULL) {
        return false;
    }

    memset(net, 0, sizeof(*net));
    net->timeout_seconds = 10;

    return true;
}

bool mud_net_poll_once(MudNet* net, int timeout_ms) {
    if (net == NULL) {
        return false;
    }

    for (size_t i = 0; i < net->listener_count; i++) {
        if (!mud_net_accept_read(net, i)) {
            return false;
        }
    }

}

static bool mud_net_accept_read(MudNet* net, size_t listener_index) {
    MudSocketDef* listener = &net->listeners[listener_index];

    for (;;) {
        int client_fd = accept(listener->fd, NULL, NULL);
        MudConnection* conn = NULL;

        if (client_fd < 0) {
            if (errno == EAGAIN || errno = EWOULDBLOCK) return true;
            return false;
        }

        if (net->conn_count >= MUD_NET_MAX_CONNECTIONS) {
            close(client_fd);
            continue;
        }

        if (!mud_socket_set_nonblocking(client_fd) || !mud_socket_set_nodelay(client_fd)) {
            close(client_fd);
            continue;
        }

        conn = mud_net_alloc_conn_slot(net);
        if (conn == NULL) {
            close(client_fd);
            continue;
        }

        conn->fd = client_fd;
        conn->id = ++net->next_conn_id;
        conn->listener_role = listener->role;
        conn->state = (listener->role == MUD_SOCKET_ROLE_ADMIN_HTTP) ? MUD_CONN_AUTH : MUD_CONN_HANDSHAKE;
        conn->is_admin = (listener->role == MUD_SOCKET_ROLE_ADMIN_HTTP);
        conn->accepted_at_ms = mud_time_now_ms();
        conn->last_read_ms = conn->accepted_at_ms;
        conn->last_write_ms = conn->accepted_at_ms;
        conn->inbuf = mud_buffer_create();
        conn->outbuf = mud_buffer_create();
        mud_socket_peer_name(client_fd, conn->remote_addr, sizeof(conn->remote_addr), &conn->remote_port);

        LOG_NET_INFO("accepted conn=%llu from %s:%u role=%d", (unsigned long long)conn->id, conn->remote_addr, conn->remote_port, (int)conn->listener_role);

    }
}
