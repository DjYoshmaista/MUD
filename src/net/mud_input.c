#include "mud_input.h"
#include "mud_log.h"
#include "mud_connection.h"
#include "mud_buffer.h"
#include "mud_output.h"

#include <ctype.h>
#include <string.h>

static bool is_printable_ascii(unsigned char ch) {
    return ch >= 32 && ch <= 126;
}

static void input_render_text(MudConnection* conn, const char* text) {
    if (conn == NULL || text == NULL || !conn->input_echo_enabled) {
        return;
    }

    mud_output_send(conn, text);
}

static void input_render_char(MudConnection* conn, unsigned char ch) {
    char buf[2];

    if (conn == NULL || !conn->input_echo_enabled) {
        return;
    }

    buf[0] = (char)ch;
    buf[1] = '\0';
    mud_output_send(conn, buf);
}

static void input_reset_escape(MudConnection* conn) {
    conn->input_in_escape = false;
    conn->escape_len = 0;
    conn->escape_buf[0] = '\0';
}

static void input_finish_line(MudConnection* conn) {
    char line[MUD_INPUT_MAX_LINE + 1];
    size_t len = mud_buffer_size(conn->input_buf);

    if (len > MUD_INPUT_MAX_LINE) {
        len = MUD_INPUT_MAX_LINE;
    }

    memcpy(line, mud_buffer_cstr(conn->input_buf), len);
    line[len] = '\0';

    input_render_text(conn, "\r\n");
    mud_buffer_clear(conn->input_buf);
    mud_input_on_line(conn, line);
}

static void input_erase_last(MudConnection* conn) {
    size_t len = mud_buffer_size(conn->input_buf);
    if (len == 0) {
        return;
    }

    mud_buffer_truncate(conn->input_buf, len - 1);
    input_render_text(conn, "\b \b");
}

static void input_process_escape_byte(MudConnection* conn, unsigned char ch) {
    if (conn->escape_len < sizeof(conn->escape_buf) - 1U) {
        conn->escape_buf[conn->escape_len++] = (char)ch;
        conn->escape_buf[conn->escape_len] = '\0';
    }

    /*
     * ESC [ ... and ESC O ... are multi-byte control sequences.
     * Do not terminate the sequence on the introducer byte itself.
     */
    if (conn->escape_len == 1U && (ch == '[' || ch == 'O')) {
        return;
    }

    if (ch >= 0x40U && ch <= 0x7eU) {
        LOG_NET_DEBUG("conn=%llu ignored escape sequence '%s'",
                      (unsigned long long)mud_connection_get_id(conn),
                      conn->escape_buf);
        input_reset_escape(conn);
    }
}

static void input_process_byte(MudConnection* conn, unsigned char ch) {
    if (conn == NULL) {
        return;
    }

    if (conn->input_in_escape) {
        input_process_escape_byte(conn, ch);
        return;
    }

    switch (ch) {
    case '\r':
        conn->input_saw_cr = true;
        input_finish_line(conn);
        return;

    case '\n':
        if (conn->input_saw_cr) {
            conn->input_saw_cr = false;
            return;
        }
        input_finish_line(conn);
        return;

    case 0x08:  // ^H - backspace
    case 0x7f:  // ^? - delete
        conn->input_saw_cr = false;
        input_erase_last(conn);
        return;

    case 0x1b:  // ^[ ANSI escape introducer
        conn->input_saw_cr = false;
        conn->input_in_escape = true;
        conn->escape_len = 0;
        conn->escape_buf[0] = '\0';
        return;

    case 0x1d:  // ^] telnet local escape (shouldn't reach server)
        conn->input_saw_cr = false;
        return;

    case '\0':
    case '\t':
        conn->input_saw_cr = false;
        return;

    default:
        conn->input_saw_cr = false;
        if (is_printable_ascii(ch)) {
            if (mud_buffer_size(conn->input_buf) < MUD_INPUT_MAX_LINE) {
                mud_buffer_append_char(conn->input_buf, (char)ch);
                input_render_char(conn, ch);
            }
        }
        return;
    }
}

void mud_input_recv(MudConnection* conn, const char* data, size_t len) {
    size_t i = 0;

    if (conn == NULL || data == NULL || len == 0) {
        return;
    }

    for (i = 0; i < len; i++) {
        input_process_byte(conn, (unsigned char)data[i]);
    }
}

void mud_input_on_line(MudConnection* conn, const char* line) {
    LOG_NET_DEBUG("conn=%llu line='%s'",
                  (unsigned long long)mud_connection_get_id(conn),
                  line != NULL ? line : "");
    if (conn == NULL || line == NULL) {
        return;
    }

    mud_output_sendf(conn, "You said: %s", line);
    mud_output_send(conn, "\r\n> ");
}
