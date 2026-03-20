/* include/mud_net.h
 * @brief Network functions
*/
#ifndef MUD_NET_H
#define MUD_NET_H

#include "mud_buffer.h"
#include "mud_socket.h"
#include "mud_session.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MUD_NET_MAX_CONNECTIONS       256
#define MUD_NET_BACKLOG               128
#define MUD_NET_MAX_LINE_BYTES        4096

typedef enum MudConnState {
    MUD_CONN_FREE = 0,
    MUD_CONN_HANDSHAKE,
    MUD_CONN_AUTH,
    MUD_CONN_ACTIVE,
    MUD_CONN_CLOSING
} MudConnState;

typedef struct MudNet {
    MudSocketDef listeners[MUD_NET_MAX_LISTENERS];
    MudConnection conns[MUD_NET_MAX_CONNECTIONS];
    size_t listener_count;
    size_t conn_count;
    uint64_t next_conn_id;
    uint32_t timeout_seconds;
} MudNet;

bool mud_net_init_from_config(MudNet* net);
bool mud_net_poll_once(MudNet* net, int timeout_ms);
void mud_net_shutdown(MudNet* net);
bool mud_net_load_listeners(MudNet* net);
MudConnection* mud_net_alloc_conn_slot(MudNet* net);
void mud_net_close_conn(MudNet* net, MudConnection* conn, const char* reason);
bool mud_net_handle_readable(MudNet* net, MudConnection* conn);
bool mud_net_handle_writable(MudNet* net, MudConnection* conn);
void mud_net_reap_timeouts(MudNet* net, uint64_t now_ms);
void mud_connection_reset(MudConnection* conn);

#ifdef __cplusplus
}
#endif

#endif // MUD_NET_H
