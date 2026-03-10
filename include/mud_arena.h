#ifndef MUD_ARENA_H
#define MUD_ARENA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MUD_ARENA_DEFAULT_ALIGN
#define MUD_ARENA_DEFAULT_ALIGN 16
#endif

typedef struct MudArena MudArena;

/*  @brief Create a new arena with the specified capacity

    @param capacity Size in bytes of the arena's memory pool
    @return Pointer to the ne warena, or NULL on allocation failure.

    The arena allocates a single contiguous block of 'capacity' bytes.
    All allocations from this arena comef rom this block.
*/
MudArena* mud_arena_create(size_t capacity);

/*  @brief Destroy an arena and free all of its memory

    @param arena Arena to destroy (NULL safe)

    After this call, all pointers obtained from this arena are invalid.
*/
void mud_arena_destroy(MudArena* arena);

/*  @brief Allocate memory from the arena with default alignment

    @param arena Arena to allocate from
    @param size Number of bytes to allocate
    @return Pointer to allocated memory, or NULL if arena exhausted

    Memory is aligned to MUD_ARENA_DEFAULT_ALIGN bytes
    Memory is NOT zero-initialized (use mud_arena_alloc_zero for that)
*/
void* mud_arena_alloc(MudArena* arena, size_t size);

/*  @brief Allocate zero-initialized memory from the arena

    @param arena Arena to allocate from
    @param size Number of bytes to allocate
    @return Pointer to zero-initialized memory, or NULL if arena exhausted
*/
void* mud_arena_alloc_zero(MudArena* arena, size_t size);

/*  @brief Allocate memory with explicity alignment

    @param arena Arena to allocate from
    @param size Number of bytes to allocate
    @param align Alignment requirement (must be power of 2)
    @return Pointer to aligned memory, or NULL if arena exhausted or invalid alignment
*/
void* mud_arena_alloc_aligned(MudArena* arena, size_t size, size_t align);

/*  @brief Allocate a single object of the given type

    Usage: MyStruct* obj = MUD_ARENA_NEW(arena, MyStruct);
*/
#define MUD_ARENA_NEW(arena, type)                                                      \
    ((type*)mud_arena_alloc_zero(arena), sizeof(type)))

/*  @brief Allocates an array of objects of the given type.

    Usage: int* numbers = MUD_ARENA_NEW_ARRAY(arena, int, 100);
*/
#define MUD_ARENA_NEW_ARRAY(arena, type, count)                                         \
    ((type*)mud_arena_alloc_zero((arena), sizeof(type) * (count)))

/*  @brief Reset the arena, making all memory available for reuse

    @param arena Arena to reset

    After reset, all previously allocated pointers are invalid
    The arena's capacity remains unchanged
    Memory contents are NOT zeroed (for perf)
*/
void mud_arena_reset(MudArena* arena);

/*  @brief Get the total capacity of the arena

    @param arena Arena to query
    @return Total capacity in bytes
*/
size_t mud_arena_capacity(const MudArena* arena);

/*  @brief Get the number of bytes currently used.

    @param arena Arena to query
    @return Used bytes (includes alignment padding)
*/
size_t mud_arena_used(const MudArena* arena);

/*  @brief Get the number of bytes remaining

    @param arena Arena to query
    @return Available bytes for allocation
*/
size_t mud_arena_remaining(const MudArena* arena);

/*  @brief Opaque type representing a position within an arena */
typedef struct MudArenaPos {
    size_t offset;
} MudArenaPos;

/*  @brief Save the current position of the arena

    @param arena Arena to query
    @return Current position that can be restored later
*/
MudArenaPos mud_arena_pos_save(const MudArena* arena);

/*  @brief Restore the arena to a previously saved position

    @param arena Arena to modify
    @param pos Position previously objtained from mud_arena_pos_save

    All allocation made after the saved position are invalidated
    The position must be from the same arena
*/
void mud_arena_pos_restore(MudArena* arena, MudArenaPos pos);

#ifdef __cplusplus
}
#endif

#endif /* MUD_ARENA_H */
