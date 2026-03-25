#ifndef MUD_CONNECTION_H
#define MUD_CONNECTION_H

#include "mud_buffer.h"

#include <libtelnet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MudSession MudSession;
typedef struct MudConnection {
    uint64_t id;
    char remote_addr[46];
    int remote_port;
    uv_tcp_t* handle;
    telnet_t* telnet;
    MudBuffer* input_buf;
    MudBuffer* output_buf;
    MudSession* session;
    bool closing;
    bool write_pending;
    bool ansi_enabled;
    bool input_echo_enabled;
    uint16_t term_width;
    uint16_t term_height;
    bool input_in_escape;
    char escape_buf[16];
    size_t escape_len;
    bool input_saw_cr;
    uint64_t connected_at_ms;
    uint64_t last_active_at_ms;
    struct MudConnection* next_global;
} MudConnection;

void mud_connection_table_init(void);
void mud_connection_table_shutdown(void);
void mud_connection_table_add(MudConnection* conn);
void mud_connection_table_remove(MudConnection* conn);
MudConnection* mud_connection_table_find(uint64_t id);
size_t mud_connection_table_count(void);
void mud_connection_table_foreach(void (*fn)(MudConnection*, void*), void* user);

MudConnection* mud_connection_create(uv_tcp_t* handle, const char* addr, int port);
void mud_connection_destroy(MudConnection* conn);
void mud_connection_close(MudConnection* conn);
void mud_connection_send_unsigned(MudConnection* conn, const unsigned char* data, size_t len);
void mud_connection_send(MudConnection* conn, const char* data, size_t len);
uint64_t mud_connection_get_id(const MudConnection* conn);
const char* mud_connection_get_addr(const MudConnection* conn);
int mud_connection_get_port(const MudConnection* conn);
void mud_connection_touch(MudConnection* conn);

#ifdef __cplusplus
}
#endif

#endif // MUD_CONNECTION_H
