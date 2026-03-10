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
    TEST_LOG_INFO("Creating arena with capacity of 1024 bytes");
    MudArena* arena = mud_arena_create(1024);
    
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena capacity: %d", mud_arena_capacity(arena));
    CHECK_INT_EQ(ctx, mud_arena_capacity(arena), 1024);
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);
    TEST_LOG_INFO("Arena used: %d", mud_arena_used(arena));
    CHECK_INT_EQ(ctx, mud_arena_remaining(arena), 1024);
    TEST_LOG_INFO("Arena remaining: %d", mud_arena_remaining(arena));

    mud_arena_destroy(arena);
}

// Test: Zero Capacity Rejected -- Tests invalid input handling
TEST(arena_create_zero_capacity) {
    MudArena* arena = mud_arena_create(0);
    TEST_LOG_INFO("Arena capacity: %d", mud_arena_capacity(arena));

    CHECK_NULL(ctx, arena);
    TEST_LOG_INFO("Arena destroyed");
}

// Test: NULL Safety -- Tests defensive NULL handling across entire API
TEST(arena_null_safety) {
    // All operations should handle NULL gracefully
    TEST_LOG_INFO("===Initial Arena Statistics===\n  - Arena Capacity: %d\n  - Arena Used: %d\n  - Arena Remaining: %d", mud_arena_capacity(NULL), mud_arena_used(NULL), mud_arena_remaining(NULL));
    CHECK_INT_EQ(ctx, mud_arena_capacity(NULL), 0);
    CHECK_INT_EQ(ctx, mud_arena_used(NULL), 0);
    CHECK_INT_EQ(ctx, mud_arena_remaining(NULL), 0);
    CHECK_NULL(ctx, mud_arena_alloc(NULL, 100));
    CHECK_NULL(ctx, mud_arena_alloc_zero(NULL, 100));
    CHECK_NULL(ctx, mud_arena_alloc_aligned(NULL, 100, 16));

    // Reset and destroy should not crash on NULL
    mud_arena_reset(NULL);
    mud_arena_destroy(NULL);

    // Position operations with NULL
    MudArenaPos pos = mud_arena_pos_save(NULL);
    CHECK_INT_EQ(ctx, pos.offset, 0);
    mud_arena_pos_restore(NULL, pos);

    CHECK(0 == 0 && "arena_null_safety completed");
    TEST_LOG_INFO("Arena Null Safety Completed");
}

// Test: Basic Allocation -- Tests sequential allocations return distinct, ordered pointers
TEST(arena_alloc_basic) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    // Allocate some memory
    TEST_LOG_INFO("Arena Created And Tested Successfully\nAllocating 64 bytes of memory to pointer ptr1");
    void* ptr1 = mud_arena_alloc(arena, 64);
    REQUIRE_NOT_NULL(ctx, ptr1);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    void* ptr2 = mud_arena_alloc(arena, 128);
    REQUIRE_NOT_NULL(ctx, ptr2);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // Pointers should be different
    CHECK(ptr1 != ptr2);

    // ptr2 should come after ptr1 (with possible alignment padding)
    CHECK((char*)ptr2 > (char*)ptr1);

    // Used should reflect allocations (may include alignment padding)
    CHECK(mud_arena_used(arena) >= 64 + 128);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Allocation (Basic) Testing Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Zero-Initialized Allocation -- Tests alloc_zero actually zeroes memory
TEST(arena_alloc_zero) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Not NULL\nBeginning Arena Allocation (Zero) Tests...\nAllocating 64 bytes of memory to pointer data");
    // Allocate zero-intiialized memory
    unsigned char* data = mud_arena_alloc_zero(arena, 64);
    REQUIRE_NOT_NULL(ctx, data);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // Verify all bytes are ezero
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) TEST_LOG_TRACE("Checking byte %d of data", i);
        CHECK_INT_EQ(ctx, data[i], 0);
        if (i % 8 == 0) TEST_LOG_TRACE("Byte %d of data successfully checked", i);
    }

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Allocation (Zero) Testing Successfully Completed!\nArena Destroyed; Memory Freed");
}

/*  Test: Alignment Guarantees -- Test Default alignment is maintained regardless of allocation size
    uintptr_t cast: allows numeric operations on pointer addresses
    Modulo check: addr % 16 == 0 means address is 16-byte aligned
*/
TEST(arena_alignment_default) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Default Alignment Test...");
    // Make several allocations of different sizes
    for (int i = 0; i < 10; i++) {
        if (i % 10 == 0) TEST_LOG_TRACE("Checking alignment of allocation of size %d", i);
        size_t size = (size_t)(i * 7 + 1); // Varying sizes: 1, 8, 15, 22, ...
        if (i % 10 == 0) TEST_LOG_TRACE("Allocating %d bytes of memory to ptr", size);
        void* ptr = mud_arena_alloc(arena, size);

        REQUIRE_NOT_NULL(ctx, ptr);
        if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

        if (i % 10 == 0) TEST_LOG_TRACE("ptr successfully tested not NULL\nChecking alignment of ptr...");
        // Check alignment
        uintptr_t addr = (uintptr_t)ptr;
        CHECK_INT_EQ(ctx, addr % MUD_ARENA_DEFAULT_ALIGN, 0);
        if (i % 10 == 0) TEST_LOG_TRACE("ptr successfully checked alignment");
    }

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Default Alignment Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Custom Alignment -- Tests custom alignment works for various power-of-two values
TEST(arena_alignment_custom) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Custom Alignment Test...");
    // Test various alignments
    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    size_t num_alignments = sizeof(alignments) / sizeof(alignments[0]);

    for (size_t i = 0; i < num_alignments; i++) {
        if (i % 9 == 0) TEST_LOG_TRACE("Checking alignment of allocation of size %d", i);
        size_t align = alignments[i];
        void* ptr = mud_arena_alloc_aligned(arena, 32, align);
        if (i % 9 == 0) TEST_LOG_TRACE("Allocating 32 bytes of memory to ptr");

        REQUIRE_NOT_NULL(ctx, ptr);
        if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
        if (i % 9 == 0) TEST_LOG_TRACE("ptr successfully tested not NULL\nChecking alignment of ptr...");

        uintptr_t addr = (uintptr_t)ptr;
        CHECK_INT_EQ(ctx, addr % align, 0);
        if (i % 9 == 0) TEST_LOG_TRACE("ptr successfully checked alignment %d", i);
    }

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Custom Alignment Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Invalid Alignment Rejected -- Tests non-power-of-two alignments are rejected
TEST(arena_alignment_invalid) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Invalid Alignment Test...");
    // Zero alignment
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 0));

    // Non-power-of-two alignments
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 3));
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 5));
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 6));
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 7));
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 9));
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 32, 12));

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Invalid Alignment Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Zero Size Allocation -- Tests zero-size allocations are rejected cleanly
TEST(arena_alloc_zero_size) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Zero Size Allocation Test...");
    // Zero-size allocation should return NULL
    CHECK_NULL(ctx, mud_arena_alloc(arena, 0));
    CHECK_NULL(ctx, mud_arena_alloc_zero(arena, 0));
    CHECK_NULL(ctx, mud_arena_alloc_aligned(arena, 0, 16));

    // Arena should be unchanged
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Zero Size Allocation Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Exhaustion behavior -- Tests Arena returns NULL when exhausted; doesn't corrupt
TEST(arena_exhaustion) {
    MudArena* arena = mud_arena_create(256);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Exhaustion Test...");
    // Allocate most of the arena
    void* ptr1 = mud_arena_alloc(arena, 200);
    CHECK_NOT_NULL(ctx, ptr1);

    // Try to allocate more than remaining
    void* ptr2 = mud_arena_alloc(arena, 100);
    CHECK_NULL(ctx, ptr2);  // Should fail

    // Small allocation might still succeed (depending on alignment padding)
    size_t remaining = mud_arena_remaining(arena);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("REMAINING: %zu.  Arena Allocation (Exhaustion) Testing Successfully Completed!\nArena Destroyed; Memory Freed", remaining);
}

// Test: Reset Operation -- Reset restores arena to initial state
TEST(arena_reset) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Reset Test...");
    // Make allocations
    mud_arena_alloc(arena, 100);
    mud_arena_alloc(arena, 200);
    mud_arena_alloc(arena, 300);

    CHECK(mud_arena_used(arena) > 0);
    size_t capacity = mud_arena_capacity(arena);

    // Reset
    mud_arena_reset(arena);
    
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);
    CHECK_INT_EQ(ctx, mud_arena_remaining(arena), capacity);
    CHECK_INT_EQ(ctx, mud_arena_capacity(arena), capacity);

    // Can allocate again
    void* ptr = mud_arena_alloc(arena, 500);
    CHECK_NOT_NULL(ctx, ptr);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Reset Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Position Save/Restore -- Tests position save/restore creates scopes without affecting earlier allocations
TEST(arena_pos_save_restore) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Position Save/Restore Test...");
    // Allocate some initial data
    int* persistent = MUD_ARENA_NEW(arena, int);
    *persistent = 42;

    size_t used_before = mud_arena_used(arena);
    MudArenaPos saved = mud_arena_pos_save(arena);

    // Allocate temp data
    mud_arena_alloc(arena, 100);
    mud_arena_alloc(arena, 200);

    CHECK(mud_arena_used(arena) > used_before);

    // Restore position
    mud_arena_pos_restore(arena, saved);

    CHECK_INT_EQ(ctx, mud_arena_used(arena), used_before);

    // Persistent data still valid
    CHECK_INT_EQ(ctx, *persistent, 42);

    // Can allocate in reclaimed space
    void* new_ptr = mud_arena_alloc(arena, 50);
    CHECK_NOT_NULL(ctx, new_ptr);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Position Save/Restore Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Temporary Scope Basic -- Tests temporary scope frees allocations made within it
TEST(arena_temp_basic) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Temporary Scope Basic Test...");
    // Allocate persistent data
    char* persistent = mud_arena_strdup(arena, "persistent");
    size_t used_after_persistent = mud_arena_used(arena);

    // Begin temporary scope
    MudArenaTemp temp = mud_arena_temp_begin(arena);

    // Allocate temporary data
    mud_arena_alloc(arena, 100);
    char* temporary = mud_arena_strdup(arena, "temporary");
    mud_arena_alloc(arena, 200);
    CHECK_INT_EQ(ctx, mud_arena_used(arena), temporary);

    CHECK(mud_arena_used(arena) > used_after_persistent);

    // End temp scope
    mud_arena_temp_end(temp);

    // Back to state after persistent allocation
    CHECK_INT_EQ(ctx, mud_arena_used(arena), used_after_persistent);

    // Persistent data still valid
    CHECK_STR_EQ(ctx, persistent, "persistent");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Temporary Scope Basic Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Nested temporary scopes -- Test scopes can nest properly -- inner ends before outer
TEST(arena_temp_nested) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Nested Temporary Scope Test...");
    size_t level0 = mud_arena_used(arena);

    MudArenaTemp outer = mud_arena_temp_begin(arena);
    mud_arena_alloc(arena, 100);
    size_t level1 = mud_arena_used(arena);
    
    {
        TEST_LOG_DEBUG("Beginning inner temporary scope");
        MudArenaTemp inner = mud_arena_temp_begin(arena);
        mud_arena_alloc(arena, 200);
        size_t level2 = mud_arena_used(arena);

        CHECK(level2 > level1);

        mud_arena_temp_end(inner);
    }
    TEST_LOG_TRACE("Inner Temporary Scope Ended");

    CHECK_INT_EQ(ctx, mud_arena_used(arena), level1);

    mud_arena_temp_end(outer);

    CHECK_INT_EQ(ctx, mud_arena_used(arena), level0);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Nested Temporary Scope Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Arena String Duplicate -- Tests str duplication creates independent copy
TEST(arena_strdup) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena String Duplicate Test...");
    const char* original = "Hello, Arena World!";
    char* copy = mud_arena_strdup(arena, original);

    REQUIRE_NOT_NULL(ctx, copy);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    CHECK_STR_EQ(ctx, copy, original);
    CHECK(copy != original);   // Different memory

    // Modify copy, original unchanged
    copy[0] = 'J';
    CHECK_STR_EQ(ctx, copy, "Jello, Arena World!");
    CHECK_STR_EQ(ctx, original, "Hello, Arena World!");

    // NULL handling
    CHECK_NULL(ctx, mud_arena_strdup(NULL, "Hello, World!"));

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena String Duplicate Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Arena String Duplicate N -- Tests length-limited string duplication with various edge cases
TEST(arena_strndup) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena String Duplicate N Test...");
    const char* original = "Hello, World!";

    // Copy exactly 5 characters
    char* partial = mud_arena_strndup(arena, original, 5);
    REQUIRE_NOT_NULL(ctx, partial);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, partial, "Hello");

    // n larger than string length
    char* full = mud_arena_strndup(arena, original, 100);
    REQUIRE_NOT_NULL(ctx, full);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, full, original);

    // n = 0
    char* empty = mud_arena_strndup(arena, original, 0);
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena String Duplicate N Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Arena Str Format -- Tests printf-style formatting in arena memory
TEST(arena_sprintf) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Str Format Test...");
    // Simple format
    char* simple = mud_arena_sprintf(arena, "Value: %d", 42);
    REQUIRE_NOT_NULL(ctx, simple);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, simple, "Value: 42");

    // Multiple arguments
    char* complex = mud_arena_sprintf(arena, "%s has %d items (%.2f%%)", "Inventory", 5, 83.33);
    REQUIRE_NOT_NULL(ctx, complex);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, complex, "Inventory has 5 items (83.33%)");

    // Empty format
    char* empty = mud_arena_sprintf(arena, "");
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Str Format Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Arena Str Concat -- Tests str concat w/ various arg counts and NULL handling
TEST(arena_strcat) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Str Concat Test...");
    // Concatenate multiple strings
    char* result = mud_arena_strcat(arena, 4, "Hello", ", ", "World", "!");
    REQUIRE_NOT_NULL(ctx, result);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, result, "Hello, World!");

    // Single string
    char* single = mud_arena_strcat(arena, 1, "alone");
    REQUIRE_NOT_NULL(ctx, single);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, single, "alone");

    // Zero count
    char* empty = mud_arena_strcat(arena, 0);
    CHECK_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // With NULL in arguemnts (treated as empty)
    char* with_null = mud_arena_strcat(arena, 3, "start", NULL, "end");
    REQUIRE_NOT_NULL(ctx, with_null);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, with_null, "startend");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Str Concat Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: arena str join -- Tests str joining w/ separator, various edge cases
TEST(arena_strjoin) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Str Join Test...");
    const char* words[] = {"apple", "banana", "cherry"};

    // Join w/ separator
    char* csv = mud_arena_strjoin(arena, ", ", words, 3);
    REQUIRE_NOT_NULL(ctx, csv);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, csv, "apple, banana, cherry");

    // Single element
    char* single = mud_arena_strjoin(arena, ", ", words, 1);
    REQUIRE_NOT_NULL(ctx, single);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, single, "apple");

    // Empty separator
    char* no_sep = mud_arena_strjoin(arena, "", words, 3);
    REQUIRE_NOT_NULL(ctx, no_sep);
    if (ctx->abort_current_test) {mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, no_sep, "applebananacherry");

    // NULL separator (treated as empty)
    char* null_sep = mud_arena_strjoin(arena, NULL, words, 3);
    REQUIRE_NOT_NULL(ctx, null_sep);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, null_sep, "applebananacherry");

    // Zero count
    char* empty = mud_arena_strjoin(arena, ", ", words, 0);
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Str Join Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Arena Substring -- Tests substring extraction with boundary clamping
TEST(arena_substr) {
    MudArena* arena = mud_arena_create(1024);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Substring Test...");
    const char* original = "Hello, World!";

    // Extract middle
    char* middle = mud_arena_substr(arena, original, 7, 5);
    REQUIRE_NOT_NULL(ctx, middle);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, middle, "World");

    // Extract from start
    char* start = mud_arena_substr(arena, original, 0, 5);
    REQUIRE_NOT_NULL(ctx, start);
    if(ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, start, "Hello");

    // Length exceeds available
    char* clamped = mud_arena_substr(arena, original, 7, 100);
    REQUIRE_NOT_NULL(ctx, clamped);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, clamped, "World!");

    // Start beyond string
    char* empty = mud_arena_substr(arena, original, 100, 5);
    REQUIRE_NOT_NULL(ctx, empty);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }
    CHECK_STR_EQ(ctx, empty, "");

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Substring Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Type-Safe Macros -- Tests type-safe allocation macros work correctly
TEST(arena_type_safe_macros) {
    MudArena* arena = mud_arena_create(4096);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Type-Safe Macros Test...");
    // Single object allocation
    typedef struct {
        int x;
        int y;
        char name[32];
    } Point;

    Point* p = MUD_ARENA_NEW(arena, Point);
    REQUIRE_NOT_NULL(ctx, p);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // Should be zero-initialized
    CHECK_INT_EQ(ctx, p->x, 0);
    CHECK_INT_EQ(ctx, p->y, 0);
    CHECK_INT_EQ(ctx, p->name[0], '\0');

    // Assign values
    p->x = 10;
    p->y = 20;
    strcpy(p->name, "origin");

    // Array allocation
    int* numbers = MUD_ARENA_NEW_ARRAY(arena, int, 100);
    REQUIRE_NOT_NULL(ctx, numbers);
    if (ctx->abort_current_test) { mud_arena_destroy(arena); return; }

    // Should be zero-initialized
    for (int i = 0; i < 100; i++) {
        CHECK_INT_EQ(ctx, numbers[i], 0);
    }

    // Use the array
    for (int i = 0; i < 100; i++) {
        numbers[i] = i * i;
    }
    CHECK_INT_EQ(ctx, numbers[10], 100);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Type-Safe Macros Test Successfully Completed!\nArena Destroyed; Memory Freed");
}

// Test: Practical usage pattern -- tests realistic command processing pattern with temp scopes
TEST(arena_practical_usage) {
    MudArena* arena = mud_arena_create(8192);
    REQUIRE_NOT_NULL(ctx, arena);
    if (ctx->abort_current_test) return;

    // Simulate receiving a command
    TEST_LOG_INFO("Arena Created And Tested Successfully\nBeginning Arena Practical Usage Test...");
    // const char* raw_command = "  say Hello, everyone! How are you?  ";

    // Process in temporary scope
    MudArenaTemp temp = mud_arena_temp_begin(arena);

    // "Parse" the command (simplified)
    // char* trimmed = mud_arena_strdup(arena, raw_command);
    // TODO: Implement actual trimming

    // Extract command verb (first word)
    char* verb = mud_arena_strndup(arena, "say", 3);
    
    // Build response
    char* response = mud_arena_sprintf(arena, "You say: '%s'", "Helllo, everyone!  How are you?");
    CHECK_STR_EQ(ctx, verb, "say");
    CHECK(response != NULL);

    mud_arena_temp_end(temp);

    // Arena is clean for next command
    CHECK_INT_EQ(ctx, mud_arena_used(arena), 0);

    mud_arena_destroy(arena);
    TEST_LOG_INFO("Arena Practical Usage Test Successfully Completed!\nArena Destroyed; Memory Freed");
}
