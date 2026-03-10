/*  @file tests/modules/test_arena.c
    @brief Unit tests for MudArena allocator system

    Tests cover:
    - Basic allocation and deallocation
    - Alignment guarantees
    - Capacity management
    - Temporary scopes
    - Arena string operations
    - Edge cases and error handling
*/

#include "test/test_autoreg.h"
#include "test/test_log.h"
#include "mud_arena.h"
#include "mud_arena_temp.h"
#include "mud_arena_string.h"
#include <string.h>
#include <stdint.h>

// Test: Creation and Destruction -- Tests basic lifecycle, initial state correctness
TEST(arena_create_destroy) {
    TEST_LOG_INFO("Creating arena with capacity of 1024 bytes\n");
    MudArena* arena = mud_arena_create(1024);
    
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena capacity: %d\n", mud_arena_capacity(arena));
    CHECK_INT_EQ(ctx, mud_arena_capacity(arena), 1024);
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);
    TEST_LOG_INFO("Arena used: %d\n", mud_arena_used(arena));
    CHECK_INT_EQ(ctx, mud_arena_remaining(arena), 1024);
    TEST_LOG_INFO("Arena remaining: %d\n", mud_arena_remaining(arena));

    mud_arena_destroy(arena);
}

// Test: Zero Capacity Rejected -- Tests invalid input handling
TEST(arena_create_zero_capacity) {
    MudArena* arena = mud_arena_create(0);
    TEST_LOG_INFO("Arena capacity: %d\n", mud_arena_capacity(arena));

    CHECK_NULL(ctx, arena);
    TEST_LOG_INFO("Arena destroyed\n");
}

// Test: NULL Safety -- Tests defensive NULL handling across entire API
TEST(arena_null_safety) {
    // All operations should handle NULL gracefully
    TEST_LOG_INFO("===Initial Arena Statistics===\n  - Arena Capacity: %d\n  - Arena Used: %d\n  - Arena Remaining: %d\n", mud_arena_capacity(NULL), mud_arena_used(NULL), mud_arena_remaining(NULL));
    CHECK_INT_EQ(ctx, mud_arena_capacity(NULL), 0);
    TEST_LOG_TRACE("Arena Capacity: %d\n", mud_arena_capacity(NULL));
    CHECK_INT_EQ(ctx, mud_arena_used(NULL), 0);
    TEST_LOG_TRACE("Arena Used: %d\n", mud_arena_used(NULL));
    CHECK_INT_EQ(ctx, mud_arena_remaining(NULL), 0);
    TEST_LOG_TRACE("Arena Remaining: %d\n", mud_arena_remaining(NULL));
    TEST_LOG_INFO("Checking NULL safety for allocation operations on arena\n\nmud_arena_alloc: ");
    CHECK_NULL(ctx, mud_arena_alloc(NULL, 100));
    TEST_LOG_TRACE("\nmud_arena_alloc_zero: ");
    CHECK_NULL(ctx, mud_arena_alloc_zero(NULL, 100));
    TEST_LOG_TRACE("\nmud_arena_alloc_aligned: ");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(NULL, 100, 16));

    // Reset and destroy should not crash on NULL
    TEST_LOG_TRACE("Resetting arena\n");
    mud_arena_reset(NULL);
    TEST_LOG_TRACE("Destroying arena\n");
    mud_arena_destroy(NULL);
    TEST_LOG_DEBUG("Arena Reset and Destroy Successfully Completed\n");

    // Position operations with NULL
    TEST_LOG_DEBUG("Checking NULL safety for position operations\nSetting position to 0 (NULL)\n");
    MudArenaPos pos = mud_arena_pos_save(NULL);
    TEST_LOG_TRACE("Position saved at offset %d\n", pos.offset);
    CHECK_INT_EQ(ctx, pos.offset, 0);
    TEST_LOG_TRACE("Restoring position to offset %d\n", pos.offset);
    mud_arena_pos_restore(NULL, pos);
    TEST_LOG_TRACE("Arena position restored to offset %d\n", pos.offset);

    TEST_LOG_TRACE("Checking NULL safety for arena allocation operations\n");
    CHECK(0 == 0 && "arena_null_safety completed");
    TEST_LOG_INFO("Arena Null Safety Completed\n");
}

// Test: Basic Allocation -- Tests sequential allocations return distinct, ordered pointers
TEST(arena_alloc_basic) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    // Allocate some memory
    TEST_LOG_INFO("Arena Created And Tested Successfully\nAllocating 64 bytes of memory to pointer ptr1\n");
    void* ptr1 = mud_arena_alloc(arena, 64);
    TEST_LOG_TRACE("Allocated 64 bytes of memory to pointer ptr1\n");
    REQUIRE_NOT_NULL(ctx, ptr1);
    TEST_LOG_TRACE("ptr1 successfully tested not NULL\n");
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    TEST_LOG_INFO("Allocating 128 bytes of memory to pointer ptr2\n");
    void* ptr2 = mud_arena_alloc(arena, 128);
    TEST_LOG_TRACE("Allocated 128 bytes of memory to pointer ptr2\n");
    REQUIRE_NOT_NULL(ctx, ptr2);
    TEST_LOG_TRACE("ptr2 successfully tested not NULL\n");
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // Pointers should be different
    TEST_LOG_DEBUG("Checking that ptr1 and ptr2 are different\n");
    CHECK(ptr1 != ptr2);
    TEST_LOG_TRACE("ptr1 and ptr2 are different\n");

    // ptr2 should come after ptr1 (with possible alignment padding)
    TEST_LOG_DEBUG("Checking that ptr2 comes after ptr1\n");
    CHECK((char*)ptr2 > (char*)ptr1);
    TEST_LOG_TRACE("ptr2 comes after ptr1\n");

    // Used should reflect allocations (may include alignment padding)
    TEST_LOG_DEBUG("Checking that used reflects allocations\n");
    CHECK(mud_arena_used(arena) >= 64 + 128);
    TEST_LOG_TRACE("Used reflects allocations\n");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Allocation (Basic) Testing Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Zero-Initialized Allocation -- Tests alloc_zero actually zeroes memory
TEST(arena_alloc_zero) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Not NULL\nBeginning Arena Allocation (Zero) Tests...\nAllocating 64 bytes of memory to pointer data\n");
    // Allocate zero-intiialized memory
    unsigned char* data = mud_arena_alloc_zero(arena, 64);
    REQUIRE_NOT_NULL(ctx, data);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // Verify all bytes are ezero
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) TEST_LOG_TRACE("Checking byte %d of data\n", i);
        CHECK_INT_EQ(ctx, data[i], 0);
        if (i % 8 == 0) TEST_LOG_TRACE("Byte %d of data successfully checked\n", i);
    }

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Allocation (Zero) Testing Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

/*  Test: Alignment Guarantees -- Test Default alignment is maintained regardless of allocation size
    uintptr_t cast: allows numeric operations on pointer addresses
    Modulo check: addr % 16 == 0 means address is 16-byte aligned
*/
TEST(arena_alignment_default) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Default Alignment Test...\n");
    // Make several allocations of different sizes
    for (int i = 0; i < 10; i++) {
        if (i % 3 == 0) TEST_LOG_TRACE("Checking alignment of allocation of size %d\n", i);
        size_t size = (size_t)(i * 7 + 1); // Varying sizes: 1, 8, 15, 22, ...
        if (i % 3 == 0) TEST_LOG_TRACE("Allocating %d bytes of memory to ptr\n", size);
        void* ptr = mud_arena_alloc(arena, size);

        REQUIRE_NOT_NULL(ctx, ptr);
        if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

        if (i % 3 == 0) TEST_LOG_TRACE("ptr successfully tested not NULL\nChecking alignment of ptr...\n");
        // Check alignment
        uintptr_t addr = (uintptr_t)ptr;
        CHECK_INT_EQ(ctx, addr % MUD_ARENA_DEFAULT_ALIGN, 0);
        if (i % 3 == 0) TEST_LOG_TRACE("ptr successfully checked alignment\n");
    }

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Default Alignment Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Custom Alignment -- Tests custom alignment works for various power-of-two values
TEST(arena_alignment_custom) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Custom Alignment Test...\n");
    // Test various alignments
    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    size_t num_alignments = sizeof(alignments) / sizeof(alignments[0]);
    TEST_LOG_DEBUG("Checking that various alignments work\n");

    for (size_t i = 0; i < num_alignments; i++) {
        if (i % 8 == 0) TEST_LOG_TRACE("Checking alignment of allocation of size %d\n", i);
        size_t align = alignments[i];
        void* ptr = mud_arena_alloc_aligned(arena, 32, align);
        if (i % 8 == 0) TEST_LOG_TRACE("Allocating 32 bytes of memory to ptr\n");

        REQUIRE_NOT_NULL(ctx, ptr);
        if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
        if (i % 8 == 0) TEST_LOG_TRACE("ptr successfully tested not NULL\nChecking alignment of ptr...\n");

        uintptr_t addr = (uintptr_t)ptr;
        CHECK_INT_EQ(ctx, addr % align, 0);
        if (i % 8 == 0) TEST_LOG_TRACE("ptr successfully checked alignment %d\n", i);
    }

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Custom Alignment Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Invalid Alignment Rejected -- Tests non-power-of-two alignments are rejected
TEST(arena_alignment_invalid) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Invalid Alignment Test...\n");
    // Zero alignment
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 0));

    // Non-power-of-two alignments
    TEST_LOG_DEBUG("Checking that non-power-of-two alignments are rejected\n");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 3));
    TEST_LOG_TRACE("Alignment of 3 rejected\n");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 5));
    TEST_LOG_TRACE("Alignment of 5 rejected\n");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 6));
    TEST_LOG_TRACE("Alignment of 6 rejected\n");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 7));
    TEST_LOG_TRACE("Alignment of 7 rejected\n");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 9));
    TEST_LOG_TRACE("Alignment of 9 rejected\n");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 12));
    TEST_LOG_TRACE("Alignment of 12 rejected\n");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Invalid Alignment Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Zero Size Allocation -- Tests zero-size allocations are rejected cleanly
TEST(arena_alloc_zero_size) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Zero Size Allocation Test...\n");
    // Zero-size allocation should return NULL
    CHECK_NULL(ctx, mud_arena_alloc(arena, 0));
    TEST_LOG_TRACE("Allocation of zero bytes of memory using mud_arena_alloc returned NULL\n");
    CHECK_NULL(ctx, mud_arena_alloc_zero(arena, 0));
    TEST_LOG_TRACE("Allocation of zero bytes of memory using mud_arena_alloc_zero returned NULL\n");
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 0, 16));
    TEST_LOG_TRACE("Allocation of zero bytes of memory aligned [16] using mud_arena_alloc_aligned returned NULL\n");

    // Arena should be unchanged
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);
    TEST_LOG_TRACE("Arena used reflects allocations\n");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Zero Size Allocation Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Exhaustion behavior -- Tests Arena returns NULL when exhausted; doesn't corrupt
TEST(arena_exhaustion) {
    MudArena* arena = mud_arena_create(256);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Exhaustion Test...\n");
    // Allocate most of the arena
    void* ptr1 = mud_arena_alloc(arena, 200);
    CHECK_NOT_NULL(ctx, ptr1);
    TEST_LOG_TRACE("Allocated 200 bytes of memory to ptr1\n");

    // Try to allocate more than remaining
    void* ptr2 = mud_arena_alloc(arena, 100);
    CHECK_NULL(ctx, ptr2);  // Should fail
    TEST_LOG_TRACE("Attempted to allocate 100 bytes of memory to ptr2\n");

    // Small allocation might still succeed (depending on alignment padding)
    TEST_LOG_DEBUG("Checking that small allocation might still succeed\n");
    size_t remaining = mud_arena_remaining(arena);
    if (remaining > 0) {
        void* ptr3 = mud_arena_alloc_aligned(arena, 1, 1);  // 1-Byte alignment
        // May or may not succeed depending on exact remaining
        if ((void*)ptr3 != NULL) {
            TEST_LOG_INFO("Allocated %d bytes to ptr3\n", remaining);
        } else if ((void*)ptr3 == NULL) {
            TEST_LOG_WARN("Failed to allocate %d bytes to ptr3\n", remaining);
        } else {
            TEST_LOG_ERROR("Unexpected allocation result for ptr3.  Value of \"remaining\": \"%d\"\n", remaining);
        }
    }

    mud_arena_destroy(arena);
}

// Test: Reset Operation -- Reset restores arena to initial state
TEST(arena_reset) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Reset Test...\n");
    // Make allocations
    mud_arena_alloc(arena, 100);
    mud_arena_alloc(arena, 200);
    mud_arena_alloc(arena, 300);
    TEST_LOG_TRACE("Allocated 100, 200 and 300 bytes of memory\n");

    CHECK(mud_arena_used(arena) > 0);
    size_t capacity = mud_arena_capacity(arena);
    TEST_LOG_TRACE("Arena capacity: %d\n", capacity);

    // Reset
    mud_arena_reset(arena);
    TEST_LOG_TRACE("Resetting arena\n");
    
    TEST_LOG_DEBUG("Checking that resetting the arena completed successfully\n");
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);
    TEST_LOG_TRACE("Arena used: %d\n", mud_arena_used(arena));
    CHECK_INT_EQ(ctx, mud_arena_remaining(arena), capacity);
    TEST_LOG_TRACE("Arena remaining: %d\n", mud_arena_remaining(arena));
    CHECK_INT_EQ(ctx, mud_arena_capacity(arena), capacity);
    TEST_LOG_TRACE("Arena capacity: %d\n", mud_arena_capacity(arena));

    // Can allocate again
    TEST_LOG_DEBUG("Checking that allocating again after resetting the arena completed successfully\n");
    void* ptr = mud_arena_alloc(arena, 500);
    TEST_LOG_TRACE("Allocated 500 bytes of memory to ptr\n");
    CHECK_NOT_NULL(ctx, ptr);
    TEST_LOG_TRACE("ptr successfully tested not NULL\n");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Reset Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Position Save/Restore -- Tests position save/restore creates scopes without affecting earlier allocations
TEST(arena_pos_save_restore) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Position Save/Restore Test...\n");
    // Allocate some initial data
    int* persistent = MUD_ARENA_NEW(arena, int);
    *persistent = 42;

    TEST_LOG_DEBUG("Checking that allocating some initial data completed successfully\n");
    size_t used_before = mud_arena_used(arena);
    MudArenaPos saved = mud_arena_pos_save(arena);
    TEST_LOG_TRACE("Saved position: %d\n", saved.offset);

    // Allocate temp data
    TEST_LOG_DEBUG("Checking that allocating temp data completed successfully\n");
    mud_arena_alloc(arena, 100);
    mud_arena_alloc(arena, 200);
    TEST_LOG_TRACE("Allocated 100 and 200 bytes of memory\n");

    CHECK(mud_arena_used(arena) > used_before);
    TEST_LOG_TRACE("Arena used: %d\nArena used before: %d", mud_arena_used(arena), used_before);

    // Restore position
    TEST_LOG_DEBUG("Checking that restoring position completed successfully\n");
    mud_arena_pos_restore(arena, saved);
    TEST_LOG_TRACE("Restored position: %d\n", saved.offset);

    TEST_LOG_DEBUG("Comparing arena used before and after restoring position\n");
    CHECK_INT_EQ(ctx, mud_arena_used(arena), used_before);
    TEST_LOG_TRACE("Arena used: %d\nArena used before: %d", mud_arena_used(arena), used_before);

    // Persistent data still valid
    TEST_LOG_DEBUG("Checking that persistent data still valid\n");
    CHECK_INT_EQ(ctx, *persistent, 42);
    TEST_LOG_TRACE("Persistent data: %d\nPersistent data before: %d", *persistent, 42);

    // Can allocate in reclaimed space
    TEST_LOG_DEBUG("Checking that allocating in reclaimed space completed successfully\n");
    void* new_ptr = mud_arena_alloc(arena, 50);
    CHECK_NOT_NULL(ctx, new_ptr);
    TEST_LOG_TRACE("Allocated 50 bytes of memory to new_ptr (and pointer not NULL)\n");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Position Save/Restore Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Temporary Scope Basic -- Tests temporary scope frees allocations made within it
TEST(arena_temp_basic) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Temporary Scope Basic Test...\n");
    // Allocate persistent data
    char* persistent = mud_arena_strdup(arena, "persistent");
    size_t used_after_persistent = mud_arena_used(arena);
    TEST_LOG_TRACE("Persistent data: %s\n", persistent);

    // Begin temporary scope
    TEST_LOG_DEBUG("Beginning temporary scope\n");
    MudArenaTemp temp = mud_arena_temp_begin(arena);

    // Allocate temporary data
    TEST_LOG_DEBUG("Allocating temporary data\n");
    mud_arena_alloc(arena, 100);
    TEST_LOG_TRACE("Allocated 100 bytes of memory\n");
    char* temporary = mud_arena_strdup(arena, "temporary");
    TEST_LOG_TRACE("Allocated string \"%s\" to temporary scope using mud_arena_strdup\n", temporary);
    mud_arena_alloc(arena, 200);
    TEST_LOG_TRACE("Allocated 200 bytes of memory\n");

    TEST_LOG_DEBUG("Comparing arena used before and after temporary scope\n");
    CHECK(mud_arena_used(arena) > used_after_persistent);
    TEST_LOG_TRACE("Arena used: %d\nArena used before: %d", mud_arena_used(arena), used_after_persistent);

    // End temp scope
    mud_arena_temp_end(temp);
    TEST_LOG_TRACE("Temporary Scope Ended\n");

    // Back to state after persistent allocation
    TEST_LOG_DEBUG("Comparing arena used before and after temporary scope\n");
    CHECK_INT_EQ(ctx, mud_arena_used(arena), used_after_persistent);
    TEST_LOG_TRACE("Arena used: %d\nArena used before: %d", mud_arena_used(arena), used_after_persistent);

    // Persistent data still valid
    TEST_LOG_DEBUG("Checking that persistent data still valid\n");
    CHECK_STR_EQ(ctx, persistent, "persistent");
    TEST_LOG_TRACE("Persistent data: %s\nPersistent data before: %s", persistent, "persistent");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Temporary Scope Basic Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Nested temporary scopes -- Test scopes can nest properly -- inner ends before outer
TEST(arena_temp_nested) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Nested Temporary Scope Test...\n");
    size_t level0 = mud_arena_used(arena);

    TEST_LOG_DEBUG("Beginning outer temporary scope\n");
    MudArenaTemp outer = mud_arena_temp_begin(arena);
    mud_arena_alloc(arena, 100);
    TEST_LOG_TRACE("Allocated 100 bytes of memory\n");
    size_t level1 = mud_arena_used(arena);
    
    {
        TEST_LOG_DEBUG("Beginning inner temporary scope\n");
        MudArenaTemp inner = mud_arena_temp_begin(arena);
        TEST_LOG_TRACE("Inner Temporary Scope Started\n");
        mud_arena_alloc(arena, 200);
        TEST_LOG_TRACE("Allocated 200 bytes of memory\n");
        size_t level2 = mud_arena_used(arena);
        TEST_LOG_TRACE("Arena used: %d\nArena used before: %d", mud_arena_used(arena), level1);

        CHECK(level2 > level1);
        TEST_LOG_TRACE("Arena used: %d\nArena used before: %d", mud_arena_used(arena), level1);

        mud_arena_temp_end(inner);
    }
    TEST_LOG_TRACE("Inner Temporary Scope Ended\n");

    CHECK_INT_EQ(ctx, mud_arena_used(arena), level1);

    mud_arena_temp_end(outer);

    CHECK_INT_EQ(ctx, mud_arena_used(arena), level0);
    TEST_LOG_TRACE("Arena used: %d\nArena used before: %d", mud_arena_used(arena), level0);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Nested Temporary Scope Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Arena String Duplicate -- Tests str duplication creates independent copy
TEST(arena_strdup) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena String Duplicate Test...\n");
    const char* original = "Hello, Arena World!";
    char* copy = mud_arena_strdup(arena, original);
    TEST_LOG_TRACE("Allocated string \"%s\" to copy using mud_arena_strdup\n", copy);

    REQUIRE_NOT_NULL(ctx, copy);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    TEST_LOG_DEBUG("Checking that copy is not NULL\n");
    CHECK_STR_EQ(ctx, copy, original);
    CHECK(copy != original);   // Different memory
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", copy, original);

    // Modify copy, original unchanged
    TEST_LOG_DEBUG("Checking that copy is modified\n");
    copy[0] = 'J';
    CHECK_STR_EQ(ctx, copy, "Jello, Arena World!");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", copy, "Jello, Arena World!");
    CHECK_STR_EQ(ctx, original, "Hello, Arena World!");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", copy, "Jello, Arena World!");

    // NULL handling
    TEST_LOG_DEBUG("Checking that NULL handling works\n");
    CHECK_NULL(ctx, mud_arena_strdup(NULL, "Hello, World!"));
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", NULL, "Hello, World!");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena String Duplicate Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Arena String Duplicate N -- Tests length-limited string duplication with various edge cases
TEST(arena_strndup) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena String Duplicate N Test...\n");
    const char* original = "Hello, World!";

    // Copy exactly 5 characters
    char* partial = mud_arena_strndup(arena, original, 5);
    TEST_LOG_TRACE("Allocated string \"%s\" to partial using mud_arena_strndup\n", partial);
    REQUIRE_NOT_NULL(ctx, partial);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, partial, "Hello");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", partial, "Hello");

    // n larger than string length
    TEST_LOG_DEBUG("Checking that n larger than string length works\n");
    char* full = mud_arena_strndup(arena, original, 100);
    REQUIRE_NOT_NULL(ctx, full);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, full, original);
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", full, original);

    // n = 0
    TEST_LOG_DEBUG("Checking that n = 0 works\n");
    char* empty = mud_arena_strndup(arena, original, 0);
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena String Duplicate N Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Arena Str Format -- Tests printf-style formatting in arena memory
TEST(arena_sprintf) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Str Format Test...\n");
    // Simple format
    char* simple = mud_arena_sprintf(arena, "Value: %d", 42);
    REQUIRE_NOT_NULL(ctx, simple);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, simple, "Value: 42");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", simple, "Value: 42");

    // Multiple arguments
    TEST_LOG_DEBUG("Checking that multiple arguments work\n");
    char* complex = mud_arena_sprintf(arena, "%s has %d items (%.2f%%)", "Inventory", 5, 83.33);
    REQUIRE_NOT_NULL(ctx, complex);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, complex, "Inventory has 5 iitems (83.33%)");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", complex, "Inventory has 5 iitems (83.33%)");

    // Empty format
    TEST_LOG_DEBUG("Checking that empty format works\n");
    char* empty = mud_arena_sprintf(arena, "");
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Str Format Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Arena Str Concat -- Tests str concat w/ various arg counts and NULL handling
TEST(arena_strcat) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Str Concat Test...\n");
    // Concatenate multiple strings
    TEST_LOG_DEBUG("Checking that multiple strings work\n");
    char* result = mud_arena_strcat(arena, 4, "Hello", ", ", "World", "!");
    REQUIRE_NOT_NULL(ctx, result);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, result, "Hello, World!");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", result, "Hello, World!");

    // Single string
    TEST_LOG_DEBUG("Checking that single string works\n");
    char* single = mud_arena_strcat(arena, 1, "alone");
    REQUIRE_NOT_NULL(ctx, single);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, single, "alone");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", single, "alone");

    // Zero count
    TEST_LOG_DEBUG("Checking that zero count works\n");
    char* empty = mud_arena_strcat(arena, 0);
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", empty, "");

    // With NULL in arguemnts (treated as empty)
    TEST_LOG_DEBUG("Checking that with NULL in arguemnts works\n");
    char* with_null = mud_arena_strcat(arena, 3, "start", NULL, "end");
    REQUIRE_NOT_NULL(ctx, with_null);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, with_null, "startend");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", with_null, "startend");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Str Concat Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: arena str join -- Tests str joining w/ separator, various edge cases
TEST(arena_strjoin) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Str Join Test...\n");
    const char* words[] = {"apple", "banana", "cherry"};

    // Join w/ separator
    TEST_LOG_DEBUG("Checking that join w/ separator works\n");
    char* csv = mud_arena_strjoin(arena, ", ", words, 3);
    REQUIRE_NOT_NULL(ctx, csv);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, csv, "apple, banana, cherry");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", csv, "apple, banana, cherry");

    // Single element
    TEST_LOG_DEBUG("Checking that single element works\n");
    char* single = mud_arena_strjoin(arena, ", ", words, 1);
    REQUIRE_NOT_NULL(ctx, single);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, single, "apple");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", single, "apple");

    // Empty separator
    TEST_LOG_DEBUG("Checking that empty separator works\n");
    char* no_sep = mud_arena_strjoin(arena, "", words, 3);
    REQUIRE_NOT_NULL(ctx, no_sep);
    if (ctx->abort_current_test) {mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, no_sep, "applebananacherry");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", no_sep, "applebananacherry");

    // NULL separator (treated as empty)
    TEST_LOG_DEBUG("Checking that NULL separator works\n");
    char* null_sep = mud_arena_strjoin(arena, NULL, words, 3);
    REQUIRE_NOT_NULL(ctx, null_sep);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, null_sep, "applebananacherry");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", null_sep, "applebananacherry");

    // Zero count
    TEST_LOG_DEBUG("Checking that zero count works\n");
    char* empty = mud_arena_strjoin(arena, ", ", words, 0);
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Str Join Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Arena Substring -- Tests substring extraction with boundary clamping
TEST(arena_substr) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Substring Test...\n");
    const char* original = "Hello, World!";

    // Extract middle
    TEST_LOG_DEBUG("Checking that middle works\n");
    char* middle = mud_arena_substr(arena, original, 7, 5);
    REQUIRE_NOT_NULL(ctx, middle);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, middle, "World");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", middle, "World");

    // Extract from start
    TEST_LOG_DEBUG("Checking that start works\n");
    char* start = mud_arena_substr(arena, original, 0, 5);
    REQUIRE_NOT_NULL(ctx, start);
    if(ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, start, "Hello");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", start, "Hello");

    // Length exceeds available
    TEST_LOG_DEBUG("Checking that length exceeds available works\n");
    char* clamped = mud_arena_substr(arena, original, 7, 100);
    REQUIRE_NOT_NULL(ctx, clamped);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, clamped, "World!");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", clamped, "World!");

    // Start beyond string
    TEST_LOG_DEBUG("Checking that start beyond string works\n");
    char* empty = mud_arena_substr(arena, original, 100, 5);
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Substring Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Type-Safe Macros -- Tests type-safe allocation macros work correctly
TEST(arena_type_safe_macros) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Type-Safe Macros Test...\n");
    // Single object allocation
    typedef struct {
        int x;
        int y;
        char name[32];
    } Point;

    TEST_LOG_DEBUG("Checking that single object allocation works\n");
    Point* p = MUD_ARENA_NEW(arena, Point);
    REQUIRE_NOT_NULL(ctx, p);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    TEST_LOG_TRACE("Allocated Point object\n");

    // Should be zero-initialized
    TEST_LOG_DEBUG("Checking that zero-initialized works\n");
    CHECK_INT_EQ(ctx, p->x, 0);
    TEST_LOG_TRACE("Point x: %d\nPoint y: %d\nPoint name: %s\n", p->x, p->y, p->name);
    CHECK_INT_EQ(ctx, p->y, 0);
    TEST_LOG_TRACE("Point x: %d\nPoint y: %d\nPoint name: %s\n", p->x, p->y, p->name);
    CHECK_INT_EQ(ctx, p->name[0], '\0');
    TEST_LOG_TRACE("Point x: %d\nPoint y: %d\nPoint name: %s\n", p->x, p->y, p->name);

    // Assign values
    TEST_LOG_DEBUG("Checking that assign values works\n");
    p->x = 10;
    TEST_LOG_TRACE("Point x: %d\nPoint y: %d\nPoint name: %s\n", p->x, p->y, p->name);
    p->y = 20;
    TEST_LOG_TRACE("Point x: %d\nPoint y: %d\nPoint name: %s\n", p->x, p->y, p->name);
    strcpy(p->name, "origin");
    TEST_LOG_TRACE("Point x: %d\nPoint y: %d\nPoint name: %s\n", p->x, p->y, p->name);

    // Array allocation
    TEST_LOG_DEBUG("Checking that array allocation works\n");
    int* numbers = MUD_ARENA_NEW_ARRAY(arena, int, 100);
    TEST_LOG_TRACE("Allocated 100 ints\n");
    REQUIRE_NOT_NULL(ctx, numbers);
    TEST_LOG_TRACE("Variable \"numbers\"[%d] successfully tested not NULL\n", numbers);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // Should be zero-initialized
    for (int i = 0; i < 100; i++) {
        if (i % 20 == 0) TEST_LOG_TRACE("Checking int %d of numbers\n", i);
        CHECK_INT_EQ(ctx, numbers[i], 0);
        if (i % 20 == 0) TEST_LOG_TRACE("%d numbers successfully checked\n", i);
    }

    // Use the array
    for (int i = 0; i < 100; i++) {
        if (i % 20 == 0) TEST_LOG_TRACE("Checking int %d of numbers\n", i);
        numbers[i] = i * i;
        if (i % 20 == 0) TEST_LOG_TRACE("%d numbers successfully checked\n", i);
    }
    CHECK_INT_EQ(ctx, numbers[10], 100);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Type-Safe Macros Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}

// Test: Practical usage pattern -- tests realistic command processing pattern with temp scopes
TEST(arena_practical_usage) {
    MudArena* arena = mud_arena_create(8192);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    // Simulate receiving a command
    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Practical Usage Test...\n");
    // const char* raw_command = "  say Hello, everyone! How are you?  ";

    // Process in temporary scope
    TEST_LOG_DEBUG("Beginning temporary scope\n");
    MudArenaTemp temp = mud_arena_temp_begin(arena);

    // "Parse" the command (simplified)
    TEST_LOG_DEBUG("Checking that \"Parse\" the command (simplified) works\n");
    // char* trimmed = mud_arena_strdup(arena, raw_command);
    // TODO: Implement actual trimming

    // Extract command verb (first word)
    TEST_LOG_DEBUG("Checking that extract command verb (first word) works\n");
    char* verb = mud_arena_strndup(arena, "say", 3);
    
    // Build response
    TEST_LOG_DEBUG("Checking that build response works\n");
    char* response = mud_arena_sprintf(arena, "You say: '%s'", "Helllo, everyone!  How are you?");
    CHECK_STR_EQ(ctx, verb, "say");
    CHECK(response != NULL);
    TEST_LOG_TRACE("Copy: %s\nOriginal: %s", response, "You say: 'Helllo, everyone!  How are you?'");

    mud_arena_temp_end(temp);
    TEST_LOG_TRACE("Temporary Scope Ended\n");

    // Arena is clean for next command
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Practical Usage Test Successfully Completed!\nArena Destroyed; Memory Freed\n");
}
