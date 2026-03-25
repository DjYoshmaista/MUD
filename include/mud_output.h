#ifndef MUD_OUTPUT_H
#define MUD_OUTPUT_H

#include "mud_connection.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MUD_ANSI_RESET      "\x1b[0m"
#define MUD_ANSI_BOLD       "\x1b[1m"
#define MUD_ANSI_UNDERLINE  "\x1b[4m"
#define MUD_ANSI_BLINK      "\x1b[5m"
#define MUD_ANSI_REVERSE    "\x1b[7m"
#define MUD_ANSI_INVISIBLE  "\x1b[8m"
#define MUD_ANSI_BLACK   "\x1b[30m"
#define MUD_ANSI_RED     "\x1b[31m"
#define MUD_ANSI_GREEN   "\x1b[32m"
#define MUD_ANSI_YELLOW  "\x1b[33m"
#define MUD_ANSI_BLUE    "\x1b[34m"
#define MUD_ANSI_MAGENTA "\x1b[35m"
#define MUD_ANSI_CYAN    "\x1b[36m"
#define MUD_ANSI_WHITE   "\x1b[37m"

void mud_output_send(MudConnection* conn, const char* text);
void mud_output_sendf(MudConnection* conn, const char* fmt, ...);
void mud_output_send_line(MudConnection* conn, const char* text);
void mud_output_send_colored(MudConnection* conn, const char* ansi_code, const char* text);
bool mud_output_supports_color(const MudConnection* conn);

#ifdef __cplusplus
}
#endif

#endif // MUD_OUTPUT_H
