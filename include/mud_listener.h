#ifndef MUD_LISTENER_H
#define MUD_LISTENER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mud_listener_start(int port, int max_connections);
void mud_listener_stop(void);

#ifdef __cplusplus
}
#endif

#endif // MUD_LISTENER_H
