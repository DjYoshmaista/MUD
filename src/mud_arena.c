#include "mud_arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Structure definition
struct MudArena {
    unsigned char* data;
    size_t capacity;
    size_t offset;
};

// Helper: Alignment calculation
static inline size_t align_up(size_t value, size_t align) {
    // Align must be power of 2
    assert((align & (align - 1)) == 0 && "alignment must be power of 2");
    return (value + align - 1) & ~(align - 1);
}

// Allocation/creation
MudArena* mud_arena_create(size_t capacity) {
    if (capacity == 0) {
        return NULL;
    }

    MudArena* arena = malloc(sizeof(MudArena));
    if (arena == NULL) {
        return NULL;
    }

    arena->data = malloc(capacity);
    if (arena->data == NULL) {
        free(arena);
        return NULL;
    }

    arena->capacity = capacity;
    arena->offset = 0;

    return arena;
}

// Destruction
void mud_arena_destroy(MudArena* arena) {
    if (arena == NULL) {
        return;
    }

    free(arena->data);
    free(arena);
}

/*  Core allocation with alignment
    - Validate inputs: NULL arena or zero size -> NULL
    - Validate alignment: Must be power of 2
    - Align the offset: Round up current position to meet alignment
    - Check capacity: Alignment might push us past end
    - Calculate new offset: Where offset will be after this allocation
    - Overflow check: `new_offset < aligned_offset` catches integer wraparound
    - Return Pointer: Address within data block
    - Update offset: Advance for next allocation
*/
void* mud_arena_alloc_aligned(MudArena* arena, size_t size, size_t align) {
    if (arena == NULL || arena->data == NULL || size == 0) {
        return NULL;
    }

    // Validate alignment is power of 2
    if (align == 0 || (align & (align - 1)) != 0) {
        return NULL;
    }

    // Calculate aligned offset
    uintptr_t base = (uintptr_t)arena->data;
    uintptr_t current = base + arena->offset;
    uintptr_t aligned = (current + align - 1) & ~((uintptr_t)align - 1);
    size_t padding = (size_t)(aligned - current);

    // Check that padding itself did not already push past capacity
    if (arena->offset > arena->capacity || padding > arena->capacity - arena->offset) {
        return NULL;
    }

    size_t aligned_offset = arena->offset + padding;

    // Cheeck for overflow and capacity
    if (size > arena->capacity - aligned_offset) {
        return NULL;  // Alignment pushed past end of arena
    }

    void* ptr = (void*)(base + aligned_offset);
    arena->offset = aligned_offset + size;

    return ptr;
}

// Default alignment wrapper: Uses the aligned version with default alignment
void* mud_arena_alloc(MudArena* arena, size_t size) {
    return mud_arena_alloc_aligned(arena, size, MUD_ARENA_DEFAULT_ALIGN);
}

// Zero-Initialized Allocation: Allocate then zero (simple composition)
void* mud_arena_alloc_zero(MudArena* arena, size_t size) {
    void* ptr = mud_arena_alloc(arena, size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

// Reset: Just reset offset.  Mmeory stays allocated but is now available for reuse
void mud_arena_reset(MudArena* arena) {
    if (arena == NULL) {
        return;
    }

    arena->offset = 0;
}

// Query functions: Access struct fields with NULL safety
size_t mud_arena_capacity(const MudArena* arena) {
    if (arena == NULL) {
        return 0;
    }
    return arena->capacity;
}

size_t mud_arena_used(const MudArena* arena) {
    if (arena == NULL) {
        return 0;
    }
    return arena->offset;
}

size_t mud_arena_remaining(const MudArena* arena) {
    if (arena == NULL) {
        return 0;
    }
    return arena->capacity - arena->offset;
}

/* Position save/restore:
    - Save: Snapshot current offset
    - Restore: Rewind to saved offset
    - Validation: Only allow restoring to earlier (or same) position.  Restoring to a later position would leave a gap
    - Silently ignoring involid position vs assert--for long running prrocess silent failure may be preferred over crash.
*/
MudArenaPos mud_arena_pos_save(const MudArena* arena) {
    MudArenaPos pos = {0};
    if (arena != NULL) {
        pos.offset = arena->offset;
    }
    return pos;
}

void mud_arena_pos_restore(MudArena* arena, MudArenaPos pos) {
    if (arena == NULL) {
        return;
    }

    // Only allow restoring to earlier position
    if (pos.offset <= arena->offset) {
        arena->offset = pos.offset;
    }
    // Silently ignore invalid positions (could assert instead)
}
