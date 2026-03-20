#ifndef MUD_SESSION_H
#define MUD_SESSION_H

#include "mud_buffer.h"
#include "mud_socket.h"
#include "mud_net.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MUD_SESSION_MAX_CONNS_PER_USER    10

typedef struct MudSession {
    mud_socket_open_listener listen_sock;
    MudSocketRole;
    MudNet net;
    MudBuffer* outbuf;
    MudBuffer* inbuf;
} MudSession;

bool mud_session_begin(MudConnection* conn, int64_t account_id, uint32_t ttl_seconds);
bool mud_session_is_valid(const MudConnection* conn, uint64_t now_ms);
void mud_session_end(MudConnection* conn);
bool mud_session_attach_account(MudConnection* conn, int64_t account_id);

#ifdef __cplusplus
}
#endif

#endif // MUD_SESSION_H
