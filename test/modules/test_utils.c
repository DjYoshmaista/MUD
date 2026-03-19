/**
 * @file test_utils.c
 * @brief Unit tests for MudVector utilities
*/

#include "test/test_autoreg.h"
#include "mud_utils.h"
#include <stdlib.h>
#include <string.h>

TEST(utils_strdup_basic) {
    const char* str = "Hello World";
    char* dupe = mud_strdup(str);

    CHECK_NOT_NULL(ctx, dupe);
    if (ctx->abort_current_test) return;

    CHECK(strcmp(str, dupe) == 0);

    free(dupe);
}

TEST(utils_strdup_null) {
    const char* str = NULL;
    char* dupe = mud_strdup(str);

    CHECK_NULL(ctx, dupe);
    if (ctx->abort_current_test) return;

    CHECK(dupe == NULL);
    free(dupe);
}

TEST(utils_strdup_empty) {
    const char* str = "";
    char* dupe = mud_strdup(str);

    CHECK_NOT_NULL(ctx, dupe);
    if (ctx->abort_current_test) return;

    CHECK(strcmp(str, dupe) == 0);
    CHECK(dupe[0] == '\0');
    free(dupe);
}
