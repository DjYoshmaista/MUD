#include "mud_telnet.h"
#include "mud_connection.h"
#include "mud_input.h"
#include "mud_log.h"

#include <string.h>

static const telnet_telopt_t mud_telopts[] = {
    { TELNET_TELOPT_ECHO,   TELNET_WILL, TELNET_DONT    },
    { TELNET_TELOPT_SGA,    TELNET_WILL, TELNET_DO      },
    { TELNET_TELOPT_NAWS,   TELNET_WONT, TELNET_DO      },
    { -1, 0, 0 }
};

static void mud_telnet_event(telnet_t* telnet, telnet_event_t* ev, void* user_data) {
    MudConnection* conn = (MudConnection*)user_data;
    (void)telnet;

    switch (ev->type) {
    case TELNET_EV_DATA:
        mud_input_recv(conn, ev->data.buffer, ev->data.size);
        break;

    case TELNET_EV_SEND:
        mud_connection_send_unsigned(conn,
                                     (const unsigned char*)ev->data.buffer,
                                     ev->data.size);
        break;

    case TELNET_EV_SUBNEGOTIATION:
        if (ev->sub.telopt == TELNET_TELOPT_NAWS && ev->sub.size == 4) {
            conn->term_width = (uint16_t)(((unsigned char)ev->sub.buffer[0] << 8) | (unsigned char)ev->sub.buffer[1]);
            conn->term_height = (uint16_t)(((unsigned char)ev->sub.buffer[2] << 8) | (unsigned char)ev->sub.buffer[3]);
            LOG_NET_DEBUG("conn=%llu negotiated NAWS %ux%u", (unsigned long long)conn->id, (unsigned)conn->term_width, (unsigned)conn->term_height);
        }
        break;

    case TELNET_EV_WARNING:
        LOG_NET_WARN("libtelnet warning on conn=%llu", (unsigned long long)conn->id);
        break;

    case TELNET_EV_ERROR:
        LOG_NET_ERROR("libtelnet error on conn=%llu", (unsigned long long)conn->id);
        mud_connection_close(conn);
        break;

    default:
        break;
    }
}

bool mud_telnet_attach(MudConnection* conn) {
    if (conn == NULL) {
        return false;
    }

    conn->telnet = telnet_init(mud_telopts, mud_telnet_event, 0, conn);
    if (conn->telnet == NULL) {
        LOG_NET_ERROR("telnet_init failed for conn=%llu", (unsigned long long)conn->id);
        return false;
    }

    telnet_negotiate(conn->telnet, TELNET_WILL, TELNET_TELOPT_SGA);
    telnet_negotiate(conn->telnet, TELNET_DO, TELNET_TELOPT_NAWS);
    telnet_negotiate(conn->telnet, TELNET_WILL, TELNET_TELOPT_ECHO);

    return true;
}

void mud_telnet_detach(MudConnection* conn) {
    if (conn != NULL && conn->telnet != NULL) {
        telnet_free(conn->telnet);
        conn->telnet = NULL;
    }
}

void mud_telnet_recv(MudConnection* conn, const char* data, size_t len) {
    if (conn == NULL || conn->telnet == NULL || data == NULL || len == 0) {
        return;
    }

    telnet_recv(conn->telnet, data, len);
}

void mud_telnet_send_str(MudConnection* conn, const char* str) {
    if (conn == NULL || str == NULL) {
        return;
    }

    if (conn->telnet != NULL) {
        telnet_send(conn->telnet, str, strlen(str));
    } else {
        mud_connection_send(conn, str, strlen(str));
    }
}

void mud_telnet_echo_off(MudConnection* conn) {
    if (conn != NULL && conn->telnet != NULL) {
        conn->input_echo_enabled = false;
        telnet_negotiate(conn->telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
    }
}

void mud_telnet_echo_on(MudConnection* conn) {
    if (conn != NULL && conn->telnet != NULL) {
        conn->input_echo_enabled = true;
        telnet_negotiate(conn->telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
    }
}
