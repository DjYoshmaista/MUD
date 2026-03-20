/**
 * @file test_crypto.c
 * @brief Unit tests for src/utils/mud_crypto.c
 */

#include "mud_crypto.h"
#include "test/test_autoreg.h"
#include "test/test_log.h"

#include <string.h>

#define TEST_CRYPTO_KEY_SIZE 16
#define TEST_SESSION_TOKEN_LEN 65

static void require_crypto_initialized(MudTestCtx* ctx) {
    REQUIRE(mud_crypto_init());
    if (ctx->abort_current_test) {
        return;
    }

    TEST_LOG_INFO("libsodium initialized for crypto test");
}

TEST(crypto_hash_password_produces_verifiable_hash) {
    require_crypto_initialized(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    char hash[CRYPTO_HASH_STR_BYTES] = {0};
    CHECK_TRUE(ctx, mud_crypto_hash_password("password", hash, sizeof(hash)));
    if (ctx->abort_current_test) {
        return;
    }

    TEST_LOG_INFO("Password: %s", "password");
    CHECK_TRUE(ctx, mud_crypto_verify_password("password", hash));
    TEST_LOG_INFO("Hashed password generated and verified");
}

TEST(crypto_verify_password_rejects_wrong_password) {
    require_crypto_initialized(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    char hash[CRYPTO_HASH_STR_BYTES] = {0};
    CHECK_TRUE(ctx, mud_crypto_hash_password("password", hash, sizeof(hash)));
    if (ctx->abort_current_test) {
        return;
    }

    TEST_LOG_INFO("Password: %s", "password");
    CHECK_FALSE(ctx, mud_crypto_verify_password("wrong-password", hash));
}

TEST(crypto_random_bytes_fills_buffer) {
    require_crypto_initialized(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    unsigned char random_bytes[TEST_CRYPTO_KEY_SIZE] = {0};
    unsigned char zeroes[TEST_CRYPTO_KEY_SIZE] = {0};

    mud_crypto_random_bytes(random_bytes, sizeof(random_bytes));

    TEST_LOG_INFO("Generated %zu random bytes", sizeof(random_bytes));
    CHECK(memcmp(random_bytes, zeroes, sizeof(random_bytes)) != 0);
}

TEST(crypto_session_token_writes_hex_string) {
    require_crypto_initialized(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    char session_token[TEST_SESSION_TOKEN_LEN] = {0};

    mud_crypto_session_token(session_token, sizeof(session_token));

    CHECK(session_token[0] != '\0');
    CHECK_INT_EQ(ctx, strlen(session_token), TEST_SESSION_TOKEN_LEN - 1);
    TEST_LOG_INFO("Session token generated: %s", session_token);
}

TEST(crypto_memzero_zeroes_buffer) {
    require_crypto_initialized(ctx);
    if (ctx->abort_current_test) {
        return;
    }

    unsigned char random_bytes[TEST_CRYPTO_KEY_SIZE] = {0};
    unsigned char zeroes[TEST_CRYPTO_KEY_SIZE] = {0};

    mud_crypto_random_bytes(random_bytes, sizeof(random_bytes));
    CHECK(memcmp(random_bytes, zeroes, sizeof(random_bytes)) != 0);
    if (ctx->abort_current_test) {
        return;
    }

    TEST_LOG_INFO("Zeroing random bytes buffer");
    mud_crypto_memzero(random_bytes, sizeof(random_bytes));
    CHECK_MEM_EQ(ctx, random_bytes, zeroes, sizeof(random_bytes));
    TEST_LOG_INFO("Random bytes buffer successfully zeroed");
}
