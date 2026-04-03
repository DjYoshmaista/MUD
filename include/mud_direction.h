#ifndef MUD_DIRECTION_H
#define MUD_DIRECTION_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MudDirection {
    MUD_DIR_INVALID = -1,
    MUD_DIR_NORTH   = 0,
    MUD_DIR_SOUTH   = 1,
    MUD_DIR_EAST    = 2,
    MUD_DIR_WEST    = 3,
    MUD_DIR_UP      = 4,
    MUD_DIR_DOWN    = 5,
    MUD_DIR_COUNT
} MudDirection;

bool            mud_direction_is_valid(MudDirection dir);
size_t          mud_direction_count(void);
const char*     mud_direction_name(MudDirection dir);
const char*     mud_direction_short(MudDirection dir);
bool            mud_direction_parse(const char* token, MudDirection* out_dir);
bool            mud_direction_opposite(MudDirection dir, MudDirection* out_opposite);
int             mud_direction_to_db(MudDirection dir);
bool            mud_direction_from_db(int value, MudDirection* out_dir);

#ifdef __cplusplus
}
#endif

#endif // MUD_DIRECTION_H
