/* include/mud_socket.h
 * @brief Socket functions
*/
#ifndef MUD_SOCKET_H
#define MUD_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MUD_NET_INVALID_FD          (-1)
#define MUD_NET_ADDRSTRLEN          64
#define MUD_NET_SESSION_TOKEN_LEN   65
#define MUD_NET_MAX_LISTENERS       4

typedef enum MudSocketRole {
    MUD_SOCKET_ROLE_TELNET = 1,
    MUD_SOCKET_ROLE_ADMIN_HTTP = 2,
} MudSocketRole;

typedef struct MudSocketDef {
    int fd;
    MudSocketRole role;
    char bind_addr[MUD_NET_ADDRSTRLEN];
    uint16_t port;
    bool enabled;
} MudSocketDef;

bool mud_socket_open_listener(
    MudSocketDef* io_def,
    const char* bind_addr,
    uint16_t port,
    int backlog
);

bool mud_socket_keepalive(int fd);
bool mud_socket_set_cloexec(int fd);
bool mud_socket_local_name(int fd, char* out_addr, size_t len, uint16_t* out_port);
bool mud_socket_query_name(int fd, char* out_addr, size_t out_addr_len, uint16_t* out_port);
void mud_socket_close(int* fd);
bool mud_socket_set_nonblocking(int fd);
bool mud_socket_set_nodelay(int fd);
bool mud_socket_peer_name(int fd, char* out_addr, size_t out_addr_len, uint16_t* out_port);

#ifdef __cplusplus
}
#endif

#endif // MUD_SOCKET_H
