/**
 * @file test_config.c
 * @brief Unit tests for mud_config.c
 */

#include "test/test_autoreg.h"
#include "test/test_log.h"
#include "mud_config.h"

static const char* kConfigPath = "config/mud.conf";

static void require_default_config_loaded(MudTestCtx* ctx) {
    REQUIRE(mud_config_load(kConfigPath));
    if (ctx->abort_current_test) {
        return;
    }

    TEST_LOG_INFO("Loaded config successfully from %s", kConfigPath);
}

TEST(config_load_file_returns_true_for_valid_config) {
    CHECK_TRUE(ctx, mud_config_load(kConfigPath));
    if (ctx->abort_current_test) {
        return;
    }

    TEST_LOG_INFO("Loaded config successfully from %s", kConfigPath);
    mud_config_shutdown();
}

TEST(config_shutdown_resets_loaded_state) {
    require_default_config_loaded(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    mud_config_shutdown();
    CHECK_STR_EQ(ctx, mud_config_get_string("server.name", "fallback"), "fallback");
    TEST_LOG_INFO("Config shutdown reset accessors to fallback values");
}

TEST(config_get_string_returns_configured_value) {
    require_default_config_loaded(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    const char* str = mud_config_get_string("server.name", "fallback");
    CHECK_STR_EQ(ctx, str, "MUDv2");

    mud_config_shutdown();
}

TEST(config_get_int_returns_configured_values) {
    require_default_config_loaded(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    CHECK_INT_EQ(ctx, mud_config_get_int("network.telnet.port", -1), 4000);
    CHECK_INT_EQ(ctx, mud_config_get_int("network.admin_http.port", -1), 6969);

    mud_config_shutdown();
}

TEST(config_get_bool_returns_configured_value) {
    require_default_config_loaded(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    CHECK_TRUE(ctx, mud_config_get_bool("network.telnet.enabled", false));

    mud_config_shutdown();
}
