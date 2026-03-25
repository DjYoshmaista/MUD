// include/mud_net_loop.h

#ifndef MUD_NET_LOOP_H
#define MUD_NET_LOOP_H

#include <stdbool.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mud_net_loop_init(void);
void mud_net_loop_run(void);
void mud_net_loop_stop(void);
void mud_net_loop_close(void);
uv_loop_t* mud_net_loop_get(void);

#ifdef __cplusplus
}
#endif

#endif
