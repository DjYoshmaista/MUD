/**
 * @file test_db.c
 * @brief Unit tests for src/utils/mud_db.c
 */

#include "mud_db.h"
#include "mud_json.h"
#include "test/test_autoreg.h"
#include "test/test_log.h"

#include <cjson/cJSON.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef MUD_TEST_FIXTURE_DB_PATH
#define MUD_TEST_FIXTURE_DB_PATH "data/db_dest/test.db"
#endif

static const char* kFixtureDbPath = MUD_TEST_FIXTURE_DB_PATH;

static void cleanup_open_db(void) {
    if (mud_db_is_open()) {
        mud_db_close();
    }
}

static void require_in_memory_db(MudTestCtx* ctx) {
    cleanup_open_db();

    REQUIRE(mud_db_open(":memory:"));
    if (ctx->abort_current_test) {
        return;
    }

    REQUIRE(mud_db_migrate());
    if (ctx->abort_current_test) {
        mud_db_close();
        return;
    }

    TEST_LOG_INFO("Opened migrated in-memory database");
}

static void require_fixture_db(MudTestCtx* ctx) {
    cleanup_open_db();

    REQUIRE(mud_db_open(kFixtureDbPath));
    if (ctx->abort_current_test) {
        return;
    }

    REQUIRE(mud_db_migrate());
    if (ctx->abort_current_test) {
        mud_db_close();
        return;
    }
}

static bool fixture_query_text(
    const char* sql,
    const char* lookup,
    char* out,
    size_t out_size
) {
    MudDbStmt* stmt = NULL;
    bool ok = false;

    if (!sql || !lookup || !out || out_size == 0) {
        return false;
    }

    out[0] = '\0';

    if (!mud_db_prepare(sql, &stmt)) {
        goto done;
    }

    if (!mud_db_bind_text(stmt, 1, lookup)) {
        goto done;
    }

    if (mud_db_step(stmt) == MUD_DB_STEP_ROW) {
        snprintf(out, out_size, "%s", mud_db_column_text(stmt, 0) ? mud_db_column_text(stmt, 0) : "");
        ok = true;
    }

done:
    mud_db_finalize(stmt);
    return ok;
}

static bool fixture_query_mixed(
    const char* item_key,
    char* out_text,
    size_t out_text_size,
    int* out_int,
    double* out_real,
    bool* out_bool
) {
    static const char* kSql =
        "SELECT text_value, int_value, real_value, bool_value "
        "FROM mixed_values WHERE item_key = ?;";
    MudDbStmt* stmt = NULL;
    bool ok = false;

    if (!item_key || !out_text || out_text_size == 0 ||
        !out_int || !out_real || !out_bool) {
        return false;
    }

    out_text[0] = '\0';

    if (!mud_db_prepare(kSql, &stmt)) {
        goto done;
    }

    if (!mud_db_bind_text(stmt, 1, item_key)) {
        goto done;
    }

    if (mud_db_step(stmt) == MUD_DB_STEP_ROW) {
        snprintf(out_text, out_text_size, "%s", mud_db_column_text(stmt, 0) ? mud_db_column_text(stmt, 0) : "");
        *out_int = mud_db_column_int(stmt, 1, 0);
        *out_real = mud_db_column_double(stmt, 2, 0.0);
        *out_bool = mud_db_column_bool(stmt, 3, false);
        ok = true;
    }

done:
    mud_db_finalize(stmt);
    return ok;
}

TEST(db_wrapper_lifecycle_and_in_memory_account_flow) {
    cleanup_open_db();

    CHECK_FALSE(ctx, mud_db_is_open());
    CHECK_FALSE(ctx, mud_db_open(NULL));
    CHECK_FALSE(ctx, mud_db_open(""));
    CHECK_FALSE(ctx, mud_db_migrate());
    CHECK_FALSE(ctx, mud_db_exec("SELECT 1;"));
    CHECK_FALSE(ctx, mud_db_account_insert("alice", "hash123"));

    require_in_memory_db(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    CHECK_TRUE(ctx, mud_db_is_open());
    CHECK_FALSE(ctx, mud_db_open(":memory:"));
    CHECK_TRUE(ctx, mud_db_migrate());

    CHECK_TRUE(ctx, mud_db_account_insert("alice", "hash123"));
    CHECK_FALSE(ctx, mud_db_account_insert("ALICE", "duplicate_hash"));

    MudDbAccount account = {0};
    CHECK_TRUE(ctx, mud_db_account_get_by_name("alice", &account));
    if (ctx->abort_current_test) {
        mud_db_close();
        return;
    }

    CHECK(account.id > 0);
    CHECK_STR_EQ(ctx, account.username, "alice");
    CHECK_STR_EQ(ctx, account.password_hash, "hash123");
    CHECK(account.created_at > 0);
    CHECK_INT_EQ(ctx, account.last_login_at, 0);
    CHECK_INT_EQ(ctx, account.failed_logins, 0);
    CHECK_FALSE(ctx, account.is_banned);

    CHECK_TRUE(ctx, mud_db_account_increment_failed_logins(account.id));
    CHECK_TRUE(ctx, mud_db_account_increment_failed_logins(account.id));

    MudDbAccount updated = {0};
    CHECK_TRUE(ctx, mud_db_account_get_by_name("alice", &updated));
    CHECK_INT_EQ(ctx, updated.failed_logins, 2);

    CHECK_TRUE(ctx, mud_db_account_update_login(account.id, 1700000000));

    MudDbAccount after_login = {0};
    CHECK_TRUE(ctx, mud_db_account_get_by_name("alice", &after_login));
    CHECK_INT_EQ(ctx, after_login.last_login_at, 1700000000);
    CHECK_INT_EQ(ctx, after_login.failed_logins, 0);

    CHECK_TRUE(ctx, mud_db_account_increment_failed_logins(account.id));
    CHECK_TRUE(ctx, mud_db_account_reset_failed_logins(account.id));

    MudDbAccount after_reset = {0};
    CHECK_TRUE(ctx, mud_db_account_get_by_name("alice", &after_reset));
    CHECK_INT_EQ(ctx, after_reset.failed_logins, 0);

    CHECK_FALSE(ctx, mud_db_account_get_by_name("missing", &after_reset));
    CHECK_FALSE(ctx, mud_db_account_update_login(9999, 1700000000));
    CHECK_FALSE(ctx, mud_db_account_increment_failed_logins(9999));
    CHECK_FALSE(ctx, mud_db_account_reset_failed_logins(9999));

    mud_db_close();
    CHECK_FALSE(ctx, mud_db_is_open());
}

TEST(db_fixture_account_matches_expected_values_and_json_projection) {
    require_fixture_db(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    MudDbAccount account = {0};
    REQUIRE(mud_db_account_get_by_name("fixture_alice", &account));
    if (ctx->abort_current_test) {
        mud_db_close();
        return;
    }

    CHECK_INT_EQ(ctx, account.id, 1);
    CHECK_STR_EQ(ctx, account.username, "fixture_alice");
    CHECK_STR_EQ(ctx, account.password_hash, "hash_alice_v1");
    CHECK_INT_EQ(ctx, account.created_at, 1700000000);
    CHECK_INT_EQ(ctx, account.last_login_at, 1700003600);
    CHECK_INT_EQ(ctx, account.failed_logins, 1);
    CHECK_FALSE(ctx, account.is_banned);

    cJSON* projected = cJSON_CreateObject();
    REQUIRE_NOT_NULL(ctx, projected);
    if (ctx->abort_current_test) {
        mud_db_close();
        return;
    }

    mud_json_set_string(projected, "username", account.username);
    mud_json_set_string(projected, "password_hash", account.password_hash);
    mud_json_set_int(projected, "created_at", (int)account.created_at);
    mud_json_set_int(projected, "last_login_at", (int)account.last_login_at);
    mud_json_set_int(projected, "failed_logins", account.failed_logins);
    mud_json_set_bool(projected, "is_banned", account.is_banned);

    char* compact = mud_json_print_compact(projected);
    REQUIRE_NOT_NULL(ctx, compact);
    if (ctx->abort_current_test) {
        mud_json_destroy(projected);
        mud_db_close();
        return;
    }

    CHECK_STR_EQ(
        ctx,
        compact,
        "{\"username\":\"fixture_alice\",\"password_hash\":\"hash_alice_v1\","
        "\"created_at\":1700000000,\"last_login_at\":1700003600,"
        "\"failed_logins\":1,\"is_banned\":false}"
    );

    cJSON* reparsed = mud_json_parse(compact);
    REQUIRE_NOT_NULL(ctx, reparsed);
    if (ctx->abort_current_test) {
        cJSON_free(compact);
        mud_json_destroy(projected);
        mud_db_close();
        return;
    }

    CHECK_STR_EQ(ctx, mud_json_get_string(reparsed, "username", "fallback"), "fixture_alice");
    CHECK_STR_EQ(ctx, mud_json_get_string(reparsed, "password_hash", "fallback"), "hash_alice_v1");
    CHECK_INT_EQ(ctx, mud_json_get_int(reparsed, "created_at", -1), 1700000000);
    CHECK_INT_EQ(ctx, mud_json_get_int(reparsed, "last_login_at", -1), 1700003600);
    CHECK_INT_EQ(ctx, mud_json_get_int(reparsed, "failed_logins", -1), 1);
    CHECK_FALSE(ctx, mud_json_get_bool(reparsed, "is_banned", true));

    mud_json_destroy(reparsed);
    cJSON_free(compact);
    mud_json_destroy(projected);
    mud_db_close();
}

TEST(db_fixture_key_values_json_documents_and_arrays_parse_correctly) {
    require_fixture_db(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    const char* key = "motd";
    char value[256] = {0};
    CHECK_TRUE(ctx, fixture_query_text(
        "SELECT cfg_value FROM app_settings WHERE cfg_key = ?;",
        key,
        value,
        sizeof(value)
    ));
    CHECK_STR_EQ(ctx, key, "motd");
    CHECK_STR_EQ(ctx, value, "Welcome to MUDv2");

    key = "region";
    memset(value, 0, sizeof(value));
    CHECK_TRUE(ctx, fixture_query_text(
        "SELECT cfg_value FROM app_settings WHERE cfg_key = ?;",
        key,
        value,
        sizeof(value)
    ));
    CHECK_STR_EQ(ctx, key, "region");
    CHECK_STR_EQ(ctx, value, "NA-Central");

    char mixed_text[128] = {0};
    int int_value = 0;
    double real_value = 0.0;
    bool bool_value = false;
    CHECK_TRUE(ctx, fixture_query_mixed(
        "alpha", mixed_text, sizeof(mixed_text), &int_value, &real_value, &bool_value
    ));
    CHECK_STR_EQ(ctx, mixed_text, "hello world");
    CHECK_INT_EQ(ctx, int_value, 42);
    CHECK(fabs(real_value - 3.14159) < 1e-9);
    CHECK_TRUE(ctx, bool_value);

    char profile_json[512] = {0};
    CHECK_TRUE(ctx, fixture_query_text(
        "SELECT json_text FROM json_documents WHERE doc_key = ?;",
        "player_profile",
        profile_json,
        sizeof(profile_json)
    ));

    cJSON* profile = mud_json_parse(profile_json);
    REQUIRE_NOT_NULL(ctx, profile);
    if (ctx->abort_current_test) {
        mud_db_close();
        return;
    }

    CHECK_STR_EQ(ctx, mud_json_get_string(profile, "username", "fallback"), "fixture_alice");
    CHECK_INT_EQ(ctx, mud_json_get_int(profile, "level", -1), 12);
    CHECK_STR_EQ(ctx, mud_json_get_string(profile, "title", "fallback"), "Builder");
    CHECK_FALSE(ctx, mud_json_get_bool(profile, "is_banned", true));

    cJSON* stats = cJSON_GetObjectItemCaseSensitive(profile, "stats");
    CHECK_NOT_NULL(ctx, stats);
    if (ctx->abort_current_test) {
        mud_json_destroy(profile);
        mud_db_close();
        return;
    }
    CHECK_INT_EQ(ctx, mud_json_get_int(stats, "str", -1), 14);
    CHECK_INT_EQ(ctx, mud_json_get_int(stats, "dex", -1), 11);

    char items_json[256] = {0};
    CHECK_TRUE(ctx, fixture_query_text(
        "SELECT array_json FROM array_documents WHERE array_key = ?;",
        "starter_items",
        items_json,
        sizeof(items_json)
    ));

    cJSON* items = mud_json_parse(items_json);
    REQUIRE_NOT_NULL(ctx, items);
    if (ctx->abort_current_test) {
        mud_json_destroy(profile);
        mud_db_close();
        return;
    }

    CHECK(cJSON_IsArray(items));
    CHECK_INT_EQ(ctx, cJSON_GetArraySize(items), 3);
    CHECK_STR_EQ(ctx, cJSON_GetArrayItem(items, 0)->valuestring, "torch");
    CHECK_STR_EQ(ctx, cJSON_GetArrayItem(items, 1)->valuestring, "rope");
    CHECK_STR_EQ(ctx, cJSON_GetArrayItem(items, 2)->valuestring, "waterskin");

    char spawns_json[256] = {0};
    CHECK_TRUE(ctx, fixture_query_text(
        "SELECT array_json FROM array_documents WHERE array_key = ?;",
        "spawn_points",
        spawns_json,
        sizeof(spawns_json)
    ));

    cJSON* spawns = mud_json_parse(spawns_json);
    REQUIRE_NOT_NULL(ctx, spawns);
    if (ctx->abort_current_test) {
        mud_json_destroy(items);
        mud_json_destroy(profile);
        mud_db_close();
        return;
    }

    CHECK(cJSON_IsArray(spawns));
    CHECK_INT_EQ(ctx, cJSON_GetArraySize(spawns), 4);
    CHECK_INT_EQ(ctx, cJSON_GetArrayItem(spawns, 0)->valueint, 101);
    CHECK_INT_EQ(ctx, cJSON_GetArrayItem(spawns, 1)->valueint, 102);
    CHECK_INT_EQ(ctx, cJSON_GetArrayItem(spawns, 2)->valueint, 103);
    CHECK_INT_EQ(ctx, cJSON_GetArrayItem(spawns, 3)->valueint, 104);

    mud_json_destroy(spawns);
    mud_json_destroy(items);
    mud_json_destroy(profile);
    mud_db_close();
}
