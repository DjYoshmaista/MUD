/**
 * @file test_hashmap.c
 * @brief Unit tests for MudHashmap implementation
 */

#include "test/test_autoreg.h"
#include "mud_hashmap.h"
#include <stdlib.h>
#include <string.h>

static int destructor_call_count = 0;

static void counting_destructor(void* value) {
    free(value);
    destructor_call_count++;
}

TEST(hashmap_create_null_safety) {
    int dummy = 1;

    MudHashmap* map = mud_hashmap_create();
    CHECK_NOT_NULL(ctx, map);
    if (ctx->abort_current_test) return;

    CHECK_INT_EQ(ctx, mud_hashmap_size(map), 0);
    CHECK(mud_hashmap_is_empty(map));

    CHECK_INT_EQ(ctx, mud_hashmap_size(NULL), 0);
    CHECK(mud_hashmap_is_empty(NULL));
    CHECK_NULL(ctx, mud_hashmap_get(NULL, "key"));
    CHECK(!mud_hashmap_has(NULL, "key"));
    CHECK(!mud_hashmap_set(NULL, "key", &dummy));
    CHECK(!mud_hashmap_remove(NULL, "key"));
    CHECK(!mud_hashmap_set(map, NULL, &dummy));
    CHECK(!mud_hashmap_has(map, NULL));
    CHECK_NULL(ctx, mud_hashmap_get(map, NULL));

    mud_hashmap_destroy(map);
    mud_hashmap_destroy(NULL);
}

TEST(hashmap_set_get_update_remove) {
    MudHashmap* map = mud_hashmap_create();
    CHECK_NOT_NULL(ctx, map);
    if (ctx->abort_current_test) return;

    int one = 100;
    int two = 200;
    int updated = 999;

    CHECK(mud_hashmap_set(map, "one", &one));
    CHECK(mud_hashmap_set(map, "two", &two));
    CHECK_INT_EQ(ctx, mud_hashmap_size(map), 2);
    CHECK_INT_EQ(ctx, *(int*)mud_hashmap_get(map, "one"), 100);
    CHECK(mud_hashmap_has(map, "two"));

    CHECK(mud_hashmap_set(map, "one", &updated));
    CHECK_INT_EQ(ctx, mud_hashmap_size(map), 2);
    CHECK_INT_EQ(ctx, *(int*)mud_hashmap_get(map, "one"), 999);

    CHECK(mud_hashmap_remove(map, "two"));
    CHECK_INT_EQ(ctx, mud_hashmap_size(map), 1);
    CHECK(!mud_hashmap_has(map, "two"));
    CHECK(!mud_hashmap_remove(map, "missing"));

    mud_hashmap_destroy(map);
}

TEST(hashmap_clear_and_empty_key) {
    MudHashmap* map = mud_hashmap_create();
    CHECK_NOT_NULL(ctx, map);
    if (ctx->abort_current_test) return;

    int a = 1;
    int b = 2;

    CHECK(mud_hashmap_set(map, "", &a));
    CHECK(mud_hashmap_set(map, "x", &b));
    CHECK(mud_hashmap_has(map, ""));
    CHECK_INT_EQ(ctx, *(int*)mud_hashmap_get(map, ""), 1);

    mud_hashmap_clear(map);
    CHECK(mud_hashmap_is_empty(map));
    CHECK_INT_EQ(ctx, mud_hashmap_size(map), 0);
    CHECK(!mud_hashmap_has(map, ""));

    mud_hashmap_destroy(map);
}

TEST(hashmap_remove_and_destroy_with_destructor) {
    MudHashmap* map = mud_hashmap_create();
    CHECK_NOT_NULL(ctx, map);
    if (ctx->abort_current_test) return;

    destructor_call_count = 0;

    int* value = malloc(sizeof(int));
    CHECK_NOT_NULL(ctx, value);
    if (ctx->abort_current_test) { mud_hashmap_destroy(map); return; }
    *value = 42;

    CHECK(mud_hashmap_set(map, "key", value));
    CHECK(mud_hashmap_remove_with(map, "key", free));
    CHECK(!mud_hashmap_has(map, "key"));

    for (int i = 0; i < 4; i++) {
        int* heap_value = malloc(sizeof(int));
        CHECK_NOT_NULL(ctx, heap_value);
        if (ctx->abort_current_test) { mud_hashmap_destroy(map); return; }
        *heap_value = i;

        char key[8];
        key[0] = (char)('a' + i);
        key[1] = '\0';
        CHECK(mud_hashmap_set(map, key, heap_value));
    }

    mud_hashmap_destroy_with(map, counting_destructor);
    CHECK_INT_EQ(ctx, destructor_call_count, 4);
}

TEST(hashmap_growth_iteration_and_keys) {
    MudHashmap* map = mud_hashmap_create();
    CHECK_NOT_NULL(ctx, map);
    if (ctx->abort_current_test) return;

    for (int i = 0; i < 32; i++) {
        int* value = malloc(sizeof(int));
        CHECK_NOT_NULL(ctx, value);
        if (ctx->abort_current_test) { mud_hashmap_destroy(map); return; }
        *value = i;

        char key[16];
        (void)snprintf(key, sizeof(key), "key_%d", i);
        CHECK(mud_hashmap_set(map, key, value));
    }

    CHECK_INT_EQ(ctx, mud_hashmap_size(map), 32);

    int seen = 0;
    MudHashmapIter iter = mud_hashmap_iter_start(map);
    while (mud_hashmap_iter_next(map, &iter)) {
        CHECK_NOT_NULL(ctx, iter.key);
        CHECK_NOT_NULL(ctx, iter.value);
        if (ctx->abort_current_test) { mud_hashmap_destroy_with(map, free); return; }
        seen++;
    }
    CHECK_INT_EQ(ctx, seen, 32);

    const char* keys[8];
    size_t count = mud_hashmap_keys(map, keys, 8);
    CHECK_INT_EQ(ctx, count, 8);

    mud_hashmap_destroy_with(map, free);
}

TEST(hashmap_key_independence) {
    MudHashmap* map = mud_hashmap_create();
    CHECK_NOT_NULL(ctx, map);
    if (ctx->abort_current_test) return;

    char key_buf[32] = "original";
    int value = 42;

    CHECK(mud_hashmap_set(map, key_buf, &value));
    strcpy(key_buf, "modified");

    CHECK(mud_hashmap_has(map, "original"));
    CHECK(!mud_hashmap_has(map, "modified"));

    mud_hashmap_destroy(map);
}
