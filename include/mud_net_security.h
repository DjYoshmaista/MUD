#ifndef MUD_NET_SECURITY_H
#define MUD_NET_SECURITY_H

#include "mud_net.h"
#include "mud_socket.h"
#include "mud_connection.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mud_net_addr_is_loopback(const char* addr);
bool mud_net_may_attempt_auth(MudConnection* conn, uint64_t now_ms);
bool mud_net_validate_line_size(const MudConnection* conn);
bool mud_net_validate_admin_origin(const MudConnection* conn);
bool mud_net_constant_time_session_check(const MudConnection* conn, const char* presented_token);

#ifdef __cplusplus
}
#endif

#endif // MUD_NET_SECURITY_H
