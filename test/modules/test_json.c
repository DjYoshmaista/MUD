/**
 * @file test_json.c
 * @brief Unit tests for src/utils/mud_json.c
 */

#include "mud_json.h"
#include "test/test_autoreg.h"
#include "test/test_log.h"
#include <cjson/cJSON.h>
#include <sqlite3.h>

#include <math.h>
#include <string.h>

static cJSON* require_parsed_json(MudTestCtx* ctx, const char* json_str) {
    cJSON* root = mud_json_parse(json_str);
    REQUIRE_NOT_NULL(ctx, root);
    if (ctx->abort_current_test) {
        return NULL;
    }

    return root;
}

TEST(json_parse_and_typed_getters_work_for_valid_object) {
    cJSON* root = require_parsed_json(
        ctx,
        "{\"name\":\"yosh\",\"age\":30,\"alive\":true,\"ratio\":3.5}"
    );
    if (ctx->abort_current_test) {
        return;
    }

    CHECK_STR_EQ(ctx, mud_json_get_string(root, "name", "fallback"), "yosh");
    CHECK_INT_EQ(ctx, mud_json_get_int(root, "age", -1), 30);
    CHECK_TRUE(ctx, mud_json_get_bool(root, "alive", false));
    CHECK(fabs(mud_json_get_double(root, "ratio", -1.0) - 3.5) < 1e-9);

    mud_json_destroy(root);
}

TEST(json_parse_returns_null_for_null_or_invalid_input) {
    CHECK_NULL(ctx, mud_json_parse(NULL));
    CHECK_NULL(ctx, mud_json_parse("{\"name\":"));
}

TEST(json_print_and_print_compact_serialize_json_objects) {
    cJSON* root = cJSON_CreateObject();
    REQUIRE_NOT_NULL(ctx, root);
    if (ctx->abort_current_test) {
        return;
    }

    mud_json_set_string(root, "name", "yosh");
    mud_json_set_int(root, "age", 30);
    mud_json_set_bool(root, "alive", true);

    char* pretty = mud_json_print(root);
    REQUIRE_NOT_NULL(ctx, pretty);
    if (ctx->abort_current_test) {
        mud_json_destroy(root);
        return;
    }

    char* compact = mud_json_print_compact(root);
    REQUIRE_NOT_NULL(ctx, compact);
    if (ctx->abort_current_test) {
        cJSON_free(pretty);
        mud_json_destroy(root);
        return;
    }

    CHECK(strstr(pretty, "\"name\"") != NULL);
    CHECK(strstr(pretty, "\"age\"") != NULL);
    CHECK(strstr(compact, "\"name\":\"yosh\"") != NULL);
    CHECK(strstr(compact, "\"age\":30") != NULL);
    CHECK(strchr(compact, '\n') == NULL);

    cJSON_free(pretty);
    cJSON_free(compact);
    mud_json_destroy(root);
}

TEST(json_getters_return_defaults_for_missing_wrong_type_and_invalid_ints) {
    cJSON* root = require_parsed_json(
        ctx,
        "{\"name\":123,\"age\":\"30\",\"active\":\"yes\",\"fraction\":3.25,\"huge\":2147483648}"
    );
    if (ctx->abort_current_test) {
        return;
    }

    CHECK_STR_EQ(ctx, mud_json_get_string(root, "missing", "fallback"), "fallback");
    CHECK_STR_EQ(ctx, mud_json_get_string(root, "name", "fallback"), "fallback");
    CHECK_INT_EQ(ctx, mud_json_get_int(root, "age", -1), -1);
    CHECK_INT_EQ(ctx, mud_json_get_int(root, "fraction", -1), -1);
    CHECK_INT_EQ(ctx, mud_json_get_int(root, "huge", -1), -1);
    CHECK_TRUE(ctx, mud_json_get_bool(root, "active", true));
    CHECK(fabs(mud_json_get_double(root, "missing_double", 4.25) - 4.25) < 1e-9);

    mud_json_destroy(root);
}

TEST(json_setters_add_fields_and_nested_objects) {
    cJSON* root = cJSON_CreateObject();
    REQUIRE_NOT_NULL(ctx, root);
    if (ctx->abort_current_test) {
        return;
    }

    cJSON* profile = cJSON_CreateObject();
    REQUIRE_NOT_NULL(ctx, profile);
    if (ctx->abort_current_test) {
        mud_json_destroy(root);
        return;
    }

    mud_json_set_string(root, "name", "yosh");
    mud_json_set_int(root, "age", 30);
    mud_json_set_double(root, "ratio", 3.5);
    mud_json_set_bool(root, "alive", true);
    mud_json_set_string(profile, "title", "builder");
    mud_json_set_object(root, "profile", profile);

    CHECK_STR_EQ(ctx, mud_json_get_string(root, "name", "fallback"), "yosh");
    CHECK_INT_EQ(ctx, mud_json_get_int(root, "age", -1), 30);
    CHECK_TRUE(ctx, mud_json_get_bool(root, "alive", false));
    CHECK(fabs(mud_json_get_double(root, "ratio", -1.0) - 3.5) < 1e-9);

    cJSON* profile_obj = cJSON_GetObjectItemCaseSensitive(root, "profile");
    CHECK_NOT_NULL(ctx, profile_obj);
    if (ctx->abort_current_test) {
        mud_json_destroy(root);
        return;
    }

    CHECK_STR_EQ(ctx, mud_json_get_string(profile_obj, "title", "fallback"), "builder");

    mud_json_destroy(root);
}

TEST(json_delete_item_removes_existing_keys) {
    cJSON* root = require_parsed_json(ctx, "{\"name\":\"yosh\",\"age\":30}");
    if (ctx->abort_current_test) {
        return;
    }

    mud_json_delete_item(root, "age");

    CHECK_INT_EQ(ctx, mud_json_get_int(root, "age", -1), -1);
    CHECK_STR_EQ(ctx, mud_json_get_string(root, "name", "fallback"), "yosh");

    mud_json_destroy(root);
}

TEST(json_null_safety_helpers_return_defaults_or_noop) {
    CHECK_NULL(ctx, mud_json_print(NULL));
    CHECK_NULL(ctx, mud_json_print_compact(NULL));
    CHECK_STR_EQ(ctx, mud_json_get_string(NULL, "name", "fallback"), "fallback");
    CHECK_INT_EQ(ctx, mud_json_get_int(NULL, "age", 7), 7);
    CHECK(fabs(mud_json_get_double(NULL, "ratio", 2.5) - 2.5) < 1e-9);
    CHECK_TRUE(ctx, mud_json_get_bool(NULL, "alive", true));

    cJSON* orphan = cJSON_CreateObject();
    REQUIRE_NOT_NULL(ctx, orphan);
    if (ctx->abort_current_test) {
        return;
    }

    mud_json_set_int(NULL, "age", 30);
    mud_json_set_double(NULL, "ratio", 3.5);
    mud_json_set_bool(NULL, "alive", true);
    mud_json_set_string(NULL, "name", "yosh");
    mud_json_set_object(NULL, "profile", orphan);
    mud_json_delete_item(NULL, "name");
    mud_json_destroy(NULL);
    mud_json_destroy(orphan);
}
