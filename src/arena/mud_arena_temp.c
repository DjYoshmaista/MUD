#include "mud_arena_temp.h"

MudArenaTemp mud_arena_temp_begin(MudArena* arena) {
    MudArenaTemp temp;
    temp.arena = arena;
    temp.saved_pos = mud_arena_pos_save(arena);
    return temp;
}

void mud_arena_temp_end(MudArenaTemp temp) {
    if (temp.arena != NULL) {
        mud_arena_pos_restore(temp.arena, temp.saved_pos);
    }
}
