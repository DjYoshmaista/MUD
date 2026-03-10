/**
 * @file test_vector.c
 * @brief Unit tests for MudVector dynamic array implementation
 */

#include "test/test_autoreg.h"
#include "mud_vector.h"
#include "test/test_log.h"

TEST(vector_create_destroy) {
    TEST_LOG_DEBUG("Creating vector with sizeof(int)");

    MudVector* vec = mud_vector_create(sizeof(int));
    CHECK_NOT_NULL(ctx, vec);
    if (ctx->abort_current_test) return;

    TEST_LOG_TRACE("Verifying initial state");
    CHECK_INT_EQ(ctx, mud_vector_size(vec), 0);
    CHECK_INT_EQ(ctx, mud_vector_capacity(vec), 8);
    CHECK(mud_vector_is_empty(vec));

    TEST_LOG_DEBUG("Destroying vector");
    mud_vector_destroy(vec);
}

TEST(vector_null_safety) {
    TEST_LOG_DEBUG("Testing NULL safety for all operations");

    int dummy = 42;

    TEST_LOG_TRACE("Testing read operations with NULL");
    CHECK_INT_EQ(ctx, mud_vector_size(NULL), 0);
    CHECK_INT_EQ(ctx, mud_vector_capacity(NULL), 0);
    CHECK(mud_vector_is_empty(NULL));
    CHECK_NULL(ctx, mud_vector_get(NULL, 0));
    CHECK_NULL(ctx, mud_vector_data(NULL));

    TEST_LOG_TRACE("Testing write operations with NULL");
    CHECK(!mud_vector_push(NULL, &dummy));
    CHECK(!mud_vector_set(NULL, 0, &dummy));
    CHECK(!mud_vector_pop(NULL, NULL));
    CHECK(!mud_vector_remove(NULL, 0));
    CHECK(!mud_vector_reserve(NULL, 10));
    CHECK(!mud_vector_shrink_to_fit(NULL));

    TEST_LOG_TRACE("Testing destroy with NULL");
    mud_vector_destroy(NULL);

    TEST_LOG_DEBUG("All NULL safety checks passed");
}

TEST(vector_push_get_set) {
    MudVector* vec = mud_vector_create(sizeof(int));
    CHECK_NOT_NULL(ctx, vec);
    if (ctx->abort_current_test) return;

    TEST_LOG_DEBUG("Pushing 10 integers");
    for (int i = 0; i < 10; i++) {
        CHECK(mud_vector_push(vec, &i));
    }

    CHECK_INT_EQ(ctx, mud_vector_size(vec), 10);
    CHECK(mud_vector_capacity(vec) >= 10);
    TEST_LOG_TRACE("Size: %zu, Capacity: %zu", mud_vector_size(vec), mud_vector_capacity(vec)); 

    for (int i = 0; i < 10; i++) {
        int* val = mud_vector_get(vec, (size_t)i);
        CHECK_NOT_NULL(ctx, val);
        if (ctx->abort_current_test) { mud_vector_destroy(vec); return; }
        CHECK_INT_EQ(ctx, *val, i);
    }

    TEST_LOG_DEBUG("Testing set operation");
    int updated = 99;
    CHECK(mud_vector_set(vec, 5, &updated));
    CHECK_INT_EQ(ctx, *(int*)mud_vector_get(vec, 5), 99);

    TEST_LOG_TRACE("Testing out-of-bounds set (should fail)");
    CHECK(!mud_vector_set(vec, 100, &updated));

    mud_vector_destroy(vec);
}

TEST(vector_insert_pop_remove) {
    MudVector* vec = mud_vector_create(sizeof(int));
    CHECK_NOT_NULL(ctx, vec);
    if (ctx->abort_current_test) return;

    TEST_LOG_DEBUG("Setting up initial values [10, 30, 40]");
    int values[] = {10, 30, 40};
    for (size_t i = 0; i < 3; i++) {
        CHECK(mud_vector_push(vec, &values[i]));
    }

    TEST_LOG_DEBUG("Inserting 20 at index 1");
    int middle = 20;
    CHECK(mud_vector_insert(vec, 1, &middle));
    CHECK_INT_EQ(ctx, mud_vector_size(vec), 4);
    CHECK_INT_EQ(ctx, *(int*)mud_vector_get(vec, 1), 20);
    for (size_t i = 0; i <= 3; i++) {
        TEST_LOG_TRACE("Vector[%d]: %d", i, *(int*)mud_vector_get(vec, i));
    }

    TEST_LOG_DEBUG("Removing element at index 2");
    CHECK(mud_vector_remove(vec, 2));
    CHECK_INT_EQ(ctx, mud_vector_size(vec), 3);
    CHECK_INT_EQ(ctx, *(int*)mud_vector_get(vec, 2), 40);
    for (size_t i = 0; i <= 2; i++) {
        TEST_LOG_TRACE("Vector[%d]: %d", i, *(int*)mud_vector_get(vec, i));
    }

    TEST_LOG_DEBUG("Popping last element");
    int popped = 0;
    CHECK(mud_vector_pop(vec, &popped));
    CHECK_INT_EQ(ctx, popped, 40);
    CHECK_INT_EQ(ctx, mud_vector_size(vec), 2);
    TEST_LOG_TRACE("Popped value: %d", popped);
    for (size_t i = 0; i <= 1; i++) {
        TEST_LOG_TRACE("Vector[%d]: %d", i, *(int*)mud_vector_get(vec, i));
    }

    TEST_LOG_TRACE("Testing invalid remove (should fail)");
    CHECK(!mud_vector_remove(vec, 10));

    mud_vector_destroy(vec);
}

TEST(vector_clear_reserve_shrink) {
    MudVector* vec = mud_vector_create(sizeof(int));
    CHECK_NOT_NULL(ctx, vec);
    if (ctx->abort_current_test) return;

    size_t initial_capacity = mud_vector_capacity(vec);
    TEST_LOG_DEBUG("Initial capacity: %zu", initial_capacity);

    TEST_LOG_DEBUG("Reserving capacity for 100 elements");
    CHECK(mud_vector_reserve(vec, 100));
    CHECK(mud_vector_capacity(vec) >= 100);
    TEST_LOG_TRACE("Capacity after reserve: %zu", mud_vector_capacity(vec));

    TEST_LOG_DEBUG("Pushing 12 elements");
    for (int i = 0; i < 12; i++) {
        CHECK(mud_vector_push(vec, &i));
    }

    TEST_LOG_DEBUG("Shrinking to fit");
    CHECK(mud_vector_shrink_to_fit(vec));
    CHECK_INT_EQ(ctx, mud_vector_capacity(vec), mud_vector_size(vec));
    TEST_LOG_TRACE("Capacity after shrink: %zu", mud_vector_capacity(vec));

    TEST_LOG_DEBUG("Clearing vector");
    mud_vector_clear(vec);
    CHECK(mud_vector_is_empty(vec));
    CHECK_INT_EQ(ctx, mud_vector_capacity(vec), 12);
    TEST_LOG_TRACE("Capacity preserved after clear: %zu", mud_vector_capacity(vec));

    TEST_LOG_DEBUG("Shrinking empty vector");
    CHECK(mud_vector_shrink_to_fit(vec));
    CHECK_INT_EQ(ctx, mud_vector_capacity(vec), initial_capacity);

    mud_vector_destroy(vec);
}

TEST(vector_data_access) {
    MudVector* vec = mud_vector_create(sizeof(int));
    CHECK_NOT_NULL(ctx, vec);
    if (ctx->abort_current_test) return;

    TEST_LOG_DEBUG("Pushing 5 elements...");
    for (size_t i = 0; i < 5; i++) {
        CHECK(mud_vector_push(vec, &i));
        TEST_LOG_TRACE("Vector[%d] checked: %d", i + 1, *(int*)mud_vector_get(vec, i));
    }

    TEST_LOG_DEBUG("Getting raw data pointer");
    int* data = mud_vector_data(vec);
    CHECK_NOT_NULL(ctx, data);
    if (ctx->abort_current_test) { mud_vector_destroy(vec); return; }

    TEST_LOG_DEBUG("Modifying via raw pointer (index 2 = 77)");
    data[2] = 77;
    CHECK_INT_EQ(ctx, *(int*)mud_vector_get(vec, 2), 77);

    TEST_LOG_TRACE("Verifying const accessor returns same address");
    CHECK_PTR_EQ(ctx, data, mud_vector_data_const(vec));

    mud_vector_destroy(vec);
}

TEST(vector_struct_elements) {
    typedef struct {
        int x;
        int y;
        char name[16];
    } Point;

    TEST_LOG_DEBUG("Creating vector of Point structs");
    MudVector* vec = mud_vector_create(sizeof(Point));
    CHECK_NOT_NULL(ctx, vec);
    if (ctx->abort_current_test) return;

    Point p1 = {10, 20, "alpha"};
    Point p2 = {30, 40, "beta"};

    TEST_LOG_DEBUG("Pushing p1 (%d, %d, '%s')", p1.x, p1.y, p1.name);
    CHECK(mud_vector_push(vec, &p1));

    TEST_LOG_DEBUG("Pushing p2 (%d, %d, '%s')", p2.x, p2.y, p2.name);
    CHECK(mud_vector_push(vec, &p2));

    TEST_LOG_DEBUG("Retrieving and verifying p1");
    Point* retrieved = mud_vector_get(vec, 0);
    CHECK_NOT_NULL(ctx, retrieved);
    if (ctx->abort_current_test) { mud_vector_destroy(vec); return; }

    CHECK_INT_EQ(ctx, retrieved->x, 10);
    CHECK_INT_EQ(ctx, retrieved->y, 20);
    CHECK_STR_EQ(ctx, retrieved->name, "alpha");

    TEST_LOG_DEBUG("Retrieving and verifying p2");
    retrieved = mud_vector_get(vec, 1);
    CHECK_INT_EQ(ctx, retrieved->x, 30);
    CHECK_STR_EQ(ctx, retrieved->name, "beta");

    mud_vector_destroy(vec);
}

TEST(vector_type_safe_macros) {
    MudVector* vec = mud_vector_create(sizeof(double));
    CHECK_NOT_NULL(ctx, vec);
    if (ctx->abort_current_test) return;

    TEST_LOG_DEBUG("Using MUD_VECTOR_PUSH macro");
    MUD_VECTOR_PUSH(vec, double, 3.14159);
    MUD_VECTOR_PUSH(vec, double, 2.71828);
    MUD_VECTOR_PUSH(vec, double, 1.41421);

    CHECK_INT_EQ(ctx, mud_vector_size(vec), 3);
    TEST_LOG_TRACE("Pushed 3 doubles: p1, e, sqrt(2)");

    TEST_LOG_DEBUG("Using MUD_VECTOR_GET macro");
    double val = MUD_VECTOR_GET(vec, double, 0);
    CHECK(val > 3.14 && val < 3.15);
    TEST_LOG_TRACE("Index 0: %f (expected ~3.14159)", val);

    val = MUD_VECTOR_GET(vec, double, 1);
    CHECK(val > 2.71 && val < 2.72);
    TEST_LOG_TRACE("Index 1: %f (expected ~2.71828)", val);

    TEST_LOG_DEBUG("Using MUD_VECTOR_GET_PTR macro");
    double* dbl_ptr = MUD_VECTOR_GET_PTR(vec, double, 2);
    CHECK_NOT_NULL(ctx, dbl_ptr);
    if (ctx->abort_current_test) { mud_vector_destroy(vec); return; }
    CHECK(*dbl_ptr > 1.41 && *dbl_ptr < 1.42);
    TEST_LOG_TRACE("Index 2 (via ptr): %f (expected ~1.41421)", *dbl_ptr);

    mud_vector_destroy(vec);
}
