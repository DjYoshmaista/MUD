#include "mud_log.h"
#include "mud_utils.h"
#include "mud_network.h"
#include "mud_crypto.h"

#include <zlog.h>
#include <sodium.h>
#include <stdio.h>
#include <string.h>

// Global variables
unsigned char SERVER_KEY[crypto_box_PUBLICKEYBYTES];
unsigned char recipient_public_key[crypto_box_PUBLICKEYBYTES];
unsigned char recipient_secret_key[crypto_box_SECRETKEYBYTES];
unsigned char sender_public_key[crypto_box_PUBLICKEYBYTES];
unsigned char sender_secret_key[crypto_box_SECRETKEYBYTES];

// Must be the first libsodium call in the entire process.  Inits CPU feature detection (AVX2, etc) for fast paths.
// Thread-safe and idempotent, calling it multiple times is fine.
// Returns 0 on success or -1 on failure (hardware too old, or PRNG unavailable)
static bool sodium_init() {
    SODIUM_FLAG = sodium_init();
    if (sodium_init() < 0) {
        return false;
    } else {
        return true;
    }
    return false;
}

static int keygen() {
    // Initialize the PRNG
    randombytes_stir();

    // Generates a random 32-byte nonce
    unsigned char nonce[crypto_box_NONCEBYTES];
    randombytes_buf(nonce, crypto_box_NONCEBYTES);

    // Generate a random 32-byte key
    unsigned char key[crypto_box_PUBLICKEYBYTES];
    randombytes_buf(key, crypto_box_PUBLICKEYBYTES);

    // Encrypts key for the recipient using nonce
    unsigned char ciphertext[crypto_box_PUBLICKEYBYTES + crypto_box_MACBYTES];
    crypto_box_easy(ciphertext, key, crypto_box_PUBLICKEYBYTES, nonce, recipient_public_key, recipient_secret_key);

    // Decrypts key using nonce
    unsigned char plaintext[crypto_box_PUBLICKEYBYTES];
    crypto_box_open_easy(plaintext, ciphertext, crypto_box_PUBLICKEYBYTES + crypto_box_MACBYTES, nonce, sender_public_key, sender_secret_key);

    // Compares the decrypted key with the original key
    if (memcmp(key, plaintext, crypto_box_PUBLICKEYBYTES) == 0) {
        // Key was successfully generated
        // Return the key
        return key;
    } else if (memcmp(key, plaintext, crypto_box_PUBLICKEYBYTES) != 0) {
        // Key was not successfully generated
        // Return -1
        return 0;
    }
    return 0;
}

void mud_log_reload(const char* config_path) {
    // Reloads the logging configuration
    if (zlog_reload(config_path) != 0) {
        LOG_ADMIN_ERROR("Failed to reload zlog config from '%s'", config_path);
    } else {
        LOG_ADMIN_INFO("Successfully reloaded config from '%s'", config_path);
    }
}

void main() {
    // Inits sodium and sets the flag
    sodium_init();

    // Generates a key and stores it in a global variable
    SERVER_KEY = keygen();

    // Saves the key to a file in /tmp/key
    LOG_CORE_INFO(g_log_core, file_fmt, "Saving key to /tmp/key");
    FILE *key_file = fopen("/tmp/key", "w");
    fwrite(SERVER_KEY, sizeof(SERVER_KEY), 1, key_file);
    fclose(key_file);

    return 0;
}    
