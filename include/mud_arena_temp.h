//  Purpose: Temporary arena scopes with automatic cleanup.  Provides RAII-like behavior in C

/*  Problem: Manual position save/restore is error prone
    Every early return needs restoration.  Missing one leaks arena space

    Solution: Scratch arenas
    A "scratch" or "temporary" arena scope:
    -   ``` MudArenatemp temp = mud_arena_temp_begin(arena);
            char* data = mud_arena_alloc(arena, 1024);
            // use data
            mud_arena_temp_end(temp);  // Restores automatically
            // data is now invalid
        ```
*/
#ifndef MUD_ARENA_TEMP_H
#define MUD_ARENA_TEMP_H
#include "mud_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/*  @brief Represents a temporary scope within an arena

    Created by mud_arena_temp_begin(), ended by mud_arena_temp_end()
    All allocation between begina nd end are freed when the scope ends
    Stores:
    - `arena`: Arena this scope belongs to
    - `saved_pos`: Position at scope start (to restore later)
    `end` function needs to know which arena to restore
*/
typedef struct MudArenaTemp {
    MudArena* arena;
    MudArenaPos saved_pos;
} MudArenaTemp;

/*  @brief Begin a temporary scope within an arena

    @param arena Arena to create temporary scope in
    @return Scope handle to pass to mud_arena_temp_end()

    Usage:  ```
        MudArenaTemp temp = mud_arena_temp_begin(arena);
        // Allocate temporary data
        mud_arena_temp_end(temp);
            ```
*/
MudArenaTemp mud_arena_temp_begin(MudArena* arena);

/*  @brief End a temporary scope, freeing all allocations made within it

    @param temp Scope handle from mud_arena_temp_begin()

    All pointers allocated between begin and end become invalid
    Symmetrical API: Begin returns handle, end takes handle
    Pass by value: MudArenaTemp is small (two machine words).  Passing by value is efficient and avoids ptr management
*/
void mud_arena_temp_end(MudArenaTemp temp);

// Macro for scoped blocks
/*  @brief Execute a block with automatic arena restoration

    Usage:  ```
        MUD_ARENA_TEMP(arena) {
            char* temp = mud_arena_alloc(arena, 1024);
            // use temp
        }
        // temp automatically invalidated, arena restored
    Do not return, break, or continue out of this block.  Use goto if early exit is needed, targetinga  label after the blcok.
*/
#define MUD_ARENA_TEMP(arena)                                               \
    for (MudArenaTemp _temp = mud_arena_temp_begin(arena), *_flag = &_temp; \
        _flag != NULL;                                                      \
        mud_arena_temp_end(_temp), _flag = NULL)

#ifdef __cplusplus
}
#endif

#endif /* MUD_ARENA_TEMP_H */
