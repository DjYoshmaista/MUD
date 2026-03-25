#include "mud_output.h"
#include "mud_telnet.h"
#include "mud_connection.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

bool mud_output_supports_color(const MudConnection* conn) {
    return conn != NULL && conn->ansi_enabled;
}

void mud_output_send(MudConnection* conn, const char* text) {
    if (conn == NULL || text == NULL) {
        return;
    }

    mud_telnet_send_str(conn, text);
}

void mud_output_sendf(MudConnection* conn, const char* fmt, ...) {
    char buffer[1024];
    va_list args;

    if (conn == NULL || fmt == NULL) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    mud_output_send(conn, buffer);
}

void mud_output_send_line(MudConnection* conn, const char* text) {
    if (text != NULL) {
        mud_output_send(conn, text);
    }
    mud_output_send(conn, "\r\n");
}

void mud_output_send_colored(MudConnection* conn, const char* ansi_code, const char* text) {
    if (!mud_output_supports_color(conn) || ansi_code == NULL) {
        mud_output_send(conn, text);
        return;
    }

    mud_output_send(conn, ansi_code);
    mud_output_send(conn, text);
    mud_output_send(conn, MUD_ANSI_RESET);
}
