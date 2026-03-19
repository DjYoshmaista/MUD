// src/utils/mud_crypto.c
#include "mud_crypto.h"
#include "mud_log.h"
#include <sodium.h>
#include <string.h>

bool mud_crypto_init(void) {
    if (sodium_init() < 0) {
        LOG_CORE_FATAL("libsodium initialization failed - secure randomness unavailable");
        return false;
    }
    LOG_CORE_INFO("libsodium initialized (version %s)", sodium_version_string());
    return true;
}

bool mud_crypto_hash_password(const char* password, char* out_hash, size_t out_hash_len) {
    if (!password || !out_hash || out_hash_len < crypto_pwhash_STRBYTES) return false;

    // crypto_pwhash_str uses Argon2id with libsodium's recommended parameters
    // OPSLIMIT_MODERATE and MEMLIMIT_MODERATE are safe defaults for game server:
    // fast enough for login, expensive enough to deter offline cracking.
    int rc = crypto_pwhash_str(out_hash, password, strlen(password), crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE);

    return rc == 0;
}

bool mud_crypto_verify_password(const char* password, const char* stored_hash) {
    if (!password || !stored_hash) return false;

    // Returns 0 if the password matches, -1 otherwise
    // This is timing-safe; it runs in constant time regardless of where the comparison fails, preventing timing side-channel attacks
    return crypto_pwhash_str_verify(stored_hash, password, strlen(password)) == 0;
}

void mud_crypto_random_bytes(void* buf, size_t len) {
    randombytes_buf(buf, len);
}

void mud_crypto_session_token(char* out_buf, size_t out_len) {
    if (out_buf == NULL || out_len < 3) {
        return;
    }

    // Number of random bytes = (out_len - 1) / 2
    size_t random_bytes = (out_len - 1) / 2;
    unsigned char raw[32];  // max 32 bytes of entropy = 64 hex chars

    if (random_bytes > sizeof(raw)) random_bytes = sizeof(raw);

    randombytes_buf(raw, random_bytes);
    sodium_bin2hex(out_buf, out_len, raw, random_bytes);
}

void mud_crypto_memzero(void* buf, size_t len) {
    sodium_memzero(buf, len);
}
