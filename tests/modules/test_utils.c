/**
 * @file test_utils.c
 * @brief Unit tests for MudVector utilities
*/

#include "test/test_autoreg.h"
#include <stdlib.h>
#include <string.h>

TEST(utils_strdup_basic) {
    const char* str = "Hello World";
    char* dup = mud_strdup(str);

    CHECK_NOT_NULL(ctx, dup);
    if (ctx->abort_current_test) return;

    CHECK(strcmp(str, dup) == 0);

    free(dup);
}

TEST(utils_strdup_null) {
    const char* str = NULL;
    char* dup = mud_strdup(str);

    CHECK_NULL(ctx, dup);
    if (ctx->abort_current_test) return;

    CHECK(dup == NULL);
    free(dup);
}

TEST(utils_strdup_empty) {
    const char* str = "";
    char* dup = mud_strdup(str);

    CHECK_NOT_NULL(ctx, dup);
    if (ctx->abort_current_test) return;

    CHECK(strcmp(str, dup) == 0);
    CHECK(dup[0] == '\0');
    free(dup);
}
