#ifndef MUD_CRYPTO_H
#define MUD_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Initialize libsodium
    Must be called once before any other mud_crypto_* function.
    @return true on success, false if the system cannot provide secure randomness
*/
bool mud_crypto_init(void);

/*  Hash a plaintext password using Argon2id

    The output is a printable null-terminated string (not raw bytes).  Includes algo, params, and salt --
    everything needed to verify the password later.  Storethis string directly in the database.

    @param password     Null-terminated plaintext password (not retained after return)
    @param out_hash     Output buffer to receive the hash string
    @param out_hash_len Size of out_hash buffer in bytes.  Must be at least CRYPTO_HASH_STR_BYTES (97 bytes)
    @return true on success, false on failure (OOM, or password too long)
*/
#define CRYPTO_HASH_STR_BYTES 128 // Generous margin over libsodium's 97 byte minimum
bool mud_crypto_hash_password(const char* password, char* out_hash, size_t out_hash_len);

/*  Verify a plaintext password against a stored hash string

    @param password     Null-terminated plaintext to check
    @param stored_hash  The hash string previously produced by mud_crypto_hash_password
    @return true if the password matches, else false.
            Returns false (non-error) for wrong passwords
*/
bool mud_crypto_verify_password(const char* password, const char* stored_hash);

/*  Fill a buffer with cryptographically secure random bytes -- for session tokens, nonces and salts */
void mud_crypto_random_bytes(void* buf, size_t len);

/*  Generate a URL-safe session token as a hex string

    @param out_buf      Output buffer
    @param out_len      Size of output buffer in bytes.  Token will be (out_len - 1) / 2 random bytes encoded as hex
                        For 32-byte token (256 bits), pass out_len = 65
*/
void mud_crypto_session_token(char* out_buf, size_t out_len);

/*  Securely zero a memory buffer
    Use this to wipe passwords and keys from memory before freeing
    The compiler is NOT allowed to optimize this out(unlike memset)
                                                     */
void mud_crypto_memzero(void* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // MUD_CRYPTO_H
