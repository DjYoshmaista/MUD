#ifndef MUD_INPUT_H
#define MUD_INPUT_H

#include "mud_connection.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MUD_INPUT_MAX_LINE 1024

void mud_input_recv(MudConnection* conn, const char* data, size_t len);
void mud_input_on_line(MudConnection* conn, const char* line);

#ifdef __cplusplus
}
#endif

#endif // MUD_INPUT_H
