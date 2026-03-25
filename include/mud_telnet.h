#ifndef MUD_TELNET_H
#define MUD_TELNET_H

#include "mud_connection.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mud_telnet_attach(MudConnection* conn);
void mud_telnet_detach(MudConnection* conn);
void mud_telnet_recv(MudConnection* conn, const char* data, size_t len);
void mud_telnet_send_str(MudConnection* conn, const char* str);
void mud_telnet_echo_off(MudConnection* conn);
void mud_telnet_echo_on(MudConnection* conn);

#ifdef __cplusplus
}
#endif

#endif // MUD_TELNET_H
