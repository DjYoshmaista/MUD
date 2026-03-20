#ifndef MUD_CONNECTION_H
#define MUD_CONNECTION_H

#include "mud_net.h"
#include "mud_socket.h"
#include "mud_session.h"
#include "mud_buffer.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MudConnection {
    int fd;
    uint64_t id;
    MudSocketRole sock_role;
    uint64_t session_expires_at;
    uint16_t failed_auth_attempts;
    uint64_t auth_locked_until_ms;
    MudConnState state;
    char remote_addr[MUD_NET_ADDRSTRLEN];
    uint16_t remote_port;
    uint64_t accepted_at_ms;
    uint64_t last_read_ms;
    uint64_t last_write_ms;
    uint64_t last_auth_attempt_ms;
    bool marked_for_close;
    bool is_admin;
    int64_t account_id;
    char session_token[MUD_NET_SESSION_TOKEN_LEN];
    MudBuffer* inbuf;
    MudBuffer* outbuf;
} MudConnection;

void mud_connection_reset(MudConnection* conn);
bool mud_connection_init(MudConnection* conn, int fd, MudSocketRole role, uint64_t conn_id);
void mud_connection_destroy(MudConnection* conn);
bool mud_connection_queue_bytes(MudConnection* conn, const void* data, size_t len);
bool mud_connection_queue_str(MudConnection* conn, const char* text);
void mud_connection_queue_flush(MudConnection* conn);

#ifdef __cplusplus
}
#endif

#endif // MUD_CONNECTION_H
