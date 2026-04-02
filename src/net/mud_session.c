#include "mud_session.h"

#include "mud_connection.h"
#include "mud_crypto.h"
#include "mud_db.h"
#include "mud_log.h"
#include "mud_output.h"
#include "mud_telnet.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

typedef enum SessionLookupResult {
    SESSION_LOOKUP_FOUND = 0,
    SESSION_LOOKUP_NOT_FOUND,
    SESSION_LOOKUP_INVALID_INPUT,
    SESSION_LOOKUP_DB_ERROR,
} SessionLookupResult;

static const int k_max_failed_attempts = 5;
static const size_t k_min_password_len = 8U;

static bool session_is_attached(const MudSession* session) {
    return session != NULL &&
           session->conn != NULL &&
           session->conn->session == session;
}

static bool session_normalize_username(const char* input, char* out, size_t out_len) {
    size_t src_len = 0;
    size_t start = 0;
    size_t end = 0;
    size_t i = 0;
    size_t normalized_len = 0;

    if (input == NULL || out == NULL || out_len == 0) {
        return false;
    }

    src_len = strlen(input);
    while (start < src_len && (input[start] == ' ' || input[start] == '\t')) {
        start++;
    }

    end = src_len;
    while (end > start && (input[end - 1U] == ' ' || input[end - 1U] == '\t')) {
        end--;
    }

    normalized_len = end - start;
    if (normalized_len == 0 || normalized_len >= out_len) {
        return false;
    }

    for (i = 0; i < normalized_len; i++) {
        unsigned char ch = (unsigned char)input[start + i];
        if (!(isalnum(ch) || ch == '_' || ch == '-')) {
            return false;
        }
        out[i] = (char)ch;
    }
    out[normalized_len] = '\0';
    return true;
}

static SessionLookupResult session_lookup_account(MudSession* session,
                                                  const char* input_username,
                                                  MudDbAccount* out_account) {
    char normalized[sizeof(session->username)];

    if (!session_is_attached(session) || input_username == NULL || out_account == NULL) {
        return SESSION_LOOKUP_INVALID_INPUT;
    }

    if (!mud_db_is_open()) {
        return SESSION_LOOKUP_DB_ERROR;
    }

    if (!session_normalize_username(input_username, normalized, sizeof(normalized))) {
        return SESSION_LOOKUP_INVALID_INPUT;
    }

    memset(out_account, 0, sizeof(*out_account));
    snprintf(session->username, sizeof(session->username), "%s", normalized);
    session->account_id = 0;
    session->failed_logins = 0;

    if (!mud_db_account_get_by_name(normalized, out_account)) {
        return SESSION_LOOKUP_NOT_FOUND;
    }

    snprintf(session->username, sizeof(session->username), "%s", out_account->username);
    session->account_id = out_account->id;
    session->failed_logins = out_account->failed_logins;
    return SESSION_LOOKUP_FOUND;
}

static bool session_authenticate_password(MudSession* session, const char* input_password) {
    MudDbAccount account = {0};
    int64_t now = 0;

    if (!session_is_attached(session) || input_password == NULL) {
        return false;
    }

    if (session->username[0] == '\0' || !mud_db_is_open()) {
        return false;
    }

    if (!mud_db_account_get_by_name(session->username, &account)) {
        LOG_AUTH_WARN("Account lookup failed for username '%s'", session->username);
        return false;
    }

    if (account.is_banned) {
        LOG_AUTH_WARN("Login denied for banned account '%s'", session->username);
        return false;
    }

    if (!mud_crypto_verify_password(input_password, account.password_hash)) {
        session->failed_logins = account.failed_logins + 1;
        (void)mud_db_account_increment_failed_logins(account.id);
        LOG_AUTH_WARN("Invalid password for username '%s' (attempt %d)",
                      session->username,
                      session->failed_logins);
        return false;
    }

    now = (int64_t)time(NULL);
    session->account_id = account.id;
    session->failed_logins = 0;
    session->last_active_at = now;
    (void)mud_db_account_update_login(account.id, now);
    (void)mud_db_account_reset_failed_logins(account.id);
    mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));

    LOG_AUTH_INFO("Authenticated username '%s' (account_id=%" PRId64 ")",
                  session->username,
                  session->account_id);
    return true;
}

static bool session_input_is_yes(const char* text) {
    return strcasecmp(text, "y") == 0 || strcasecmp(text, "yes") == 0;
}

static bool session_input_is_no(const char* text) {
    return strcasecmp(text, "n") == 0 || strcasecmp(text, "no") == 0;
}

MudSession* mud_session_create(MudConnection* conn) {
    MudSession* session = NULL;

    if (conn == NULL || conn->session != NULL || conn->id == 0 || conn->closing) {
        return NULL;
    }

    session = calloc(1, sizeof(*session));
    if (session == NULL) {
        LOG_SESSION_ERROR("Failed to allocate session for conn=%llu",
                          (unsigned long long)conn->id);
        return NULL;
    }

    session->conn = conn;
    session->state = CONN_NEW;
    conn->session = session;
    mud_session_transition(session, CONN_NEW);

    LOG_SESSION_INFO("Created session for conn=%llu", (unsigned long long)conn->id);
    return session;
}

void mud_session_destroy(MudSession* session) {
    if (session == NULL) {
        return;
    }

    mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));
    if (session->conn != NULL && session->conn->session == session) {
        session->conn->session = NULL;
    }
    free(session);
}

void mud_session_on_line(MudSession* session, const char* line) {
    MudDbAccount account = {0};
    char input[1025];
    char hash[CRYPTO_HASH_STR_BYTES] = {0};
    char* p = NULL;
    char* e = NULL;
    int64_t now = 0;
    SessionLookupResult lookup = SESSION_LOOKUP_INVALID_INPUT;

    if (!session_is_attached(session) || line == NULL) {
        return;
    }

    now = (int64_t)time(NULL);
    session->last_active_at = now;
    mud_connection_touch(session->conn);

    snprintf(input, sizeof(input), "%s", line);
    p = input;
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    e = p + strlen(p);
    while (e > p && (e[-1] == ' ' || e[-1] == '\t')) {
        e--;
    }
    *e = '\0';

    switch (session->state) {
    case CONN_NEW:
        mud_session_transition(session, CONN_GET_NAME);
        return;

    case CONN_GET_NAME:
        if (p[0] == '\0') {
            mud_session_transition(session, CONN_GET_NAME);
            return;
        }

        lookup = session_lookup_account(session, p, &account);
        if (lookup == SESSION_LOOKUP_FOUND) {
            mud_session_transition(session, CONN_GET_PASSWORD);
            return;
        }
        if (lookup == SESSION_LOOKUP_NOT_FOUND) {
            mud_session_transition(session, CONN_NEW_ACCOUNT);
            return;
        }
        if (lookup == SESSION_LOOKUP_INVALID_INPUT) {
            mud_output_send_line(session->conn,
                                 "Invalid username. Use letters, numbers, '_' or '-'.");
            mud_session_transition(session, CONN_GET_NAME);
            return;
        }

        mud_output_send_line(session->conn, "Database unavailable. Disconnecting.");
        mud_session_transition(session, CONN_DISCONNECTING);
        return;

    case CONN_GET_PASSWORD:
        if (p[0] == '\0') {
            mud_session_transition(session, CONN_GET_PASSWORD);
            return;
        }

        if (session_authenticate_password(session, p)) {
            mud_session_transition(session, CONN_LIMBO);
            return;
        }

        if (session->failed_logins >= k_max_failed_attempts) {
            mud_output_send_line(session->conn, "Too many failed attempts.");
            mud_session_transition(session, CONN_DISCONNECTING);
            return;
        }

        mud_output_send_line(session->conn, "Invalid password.");
        mud_session_transition(session, CONN_GET_PASSWORD);
        return;

    case CONN_NEW_ACCOUNT:
        if (session_input_is_yes(p)) {
            mud_session_transition(session, CONN_SET_PASSWORD);
            return;
        }
        if (session_input_is_no(p)) {
            session->username[0] = '\0';
            session->account_id = 0;
            session->failed_logins = 0;
            mud_session_transition(session, CONN_GET_NAME);
            return;
        }
        mud_output_send(session->conn, "Please answer y or n: ");
        return;

    case CONN_SET_PASSWORD:
        if (p[0] == '\0') {
            mud_session_transition(session, CONN_SET_PASSWORD);
            return;
        }
        if (strlen(p) < k_min_password_len) {
            mud_output_sendf(session->conn,
                             "Password must be at least %zu characters.\r\nChoose new password: ",
                             k_min_password_len);
            return;
        }
        if (strlen(p) >= sizeof(session->pw_buf)) {
            mud_output_send(session->conn, "Password too long.\r\nChoose new password: ");
            return;
        }
        memcpy(session->pw_buf, p, strlen(p) + 1U);
        mud_session_transition(session, CONN_CONFIRM_PASSWORD);
        return;

    case CONN_CONFIRM_PASSWORD:
        if (strcmp(p, session->pw_buf) != 0) {
            mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));
            mud_output_send_line(session->conn, "Passwords do not match.");
            mud_session_transition(session, CONN_SET_PASSWORD);
            return;
        }

        if (!mud_crypto_hash_password(session->pw_buf, hash, sizeof(hash))) {
            mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));
            mud_output_send_line(session->conn, "Could not hash password.");
            mud_session_transition(session, CONN_DISCONNECTING);
            return;
        }

        if (!mud_db_account_insert(session->username, hash)) {
            mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));
            mud_crypto_memzero(hash, sizeof(hash));
            mud_output_send_line(session->conn,
                                 "Could not create account. Username may already exist.");
            mud_session_transition(session, CONN_GET_NAME);
            return;
        }

        if (!mud_db_account_get_by_name(session->username, &account)) {
            mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));
            mud_crypto_memzero(hash, sizeof(hash));
            mud_output_send_line(session->conn, "Account created, but lookup failed.");
            mud_session_transition(session, CONN_DISCONNECTING);
            return;
        }

        session->account_id = account.id;
        session->failed_logins = 0;
        (void)mud_db_account_update_login(account.id, now);
        (void)mud_db_account_reset_failed_logins(account.id);
        mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));
        mud_crypto_memzero(hash, sizeof(hash));
        mud_session_transition(session, CONN_LIMBO);
        return;

    case CONN_LIMBO:
        if (strcasecmp(p, "enter") == 0) {
            mud_session_transition(session, CONN_PLAYING);
            return;
        }
        if (strcasecmp(p, "quit") == 0) {
            mud_session_transition(session, CONN_DISCONNECTING);
            return;
        }
        mud_output_send(session->conn, "Type 'enter' or 'quit'.\r\n> ");
        return;

    case CONN_PLAYING:
        if (strcasecmp(p, "quit") == 0) {
            mud_session_transition(session, CONN_DISCONNECTING);
            return;
        }
        mud_output_sendf(session->conn, "You said: %s\r\n> ", p);
        return;

    case CONN_DISCONNECTING:
        return;
    }
}

void mud_session_transition(MudSession* session, SessionState new_state) {
    SessionState old_state = CONN_DISCONNECTING;
    int64_t now = 0;

    if (!session_is_attached(session)) {
        return;
    }

    old_state = session->state;
    now = (int64_t)time(NULL);
    session->last_active_at = now;
    session->state = new_state;

    LOG_SESSION_INFO("Session conn=%llu state %d -> %d",
                     (unsigned long long)session->conn->id,
                     (int)old_state,
                     (int)new_state);

    switch (new_state) {
    case CONN_NEW:
        session->connected_at = now;
        session->failed_logins = 0;
        session->account_id = 0;
        session->username[0] = '\0';
        mud_crypto_memzero(session->pw_buf, sizeof(session->pw_buf));
        session->player = NULL;
        mud_session_transition(session, CONN_GET_NAME);
        return;

    case CONN_GET_NAME:
        mud_telnet_echo_on(session->conn);
        mud_output_send(session->conn, "Username: ");
        return;

    case CONN_GET_PASSWORD:
        mud_telnet_echo_off(session->conn);
        mud_output_send(session->conn, "Password: ");
        return;

    case CONN_NEW_ACCOUNT:
        mud_telnet_echo_on(session->conn);
        mud_output_sendf(session->conn,
                         "No account named '%s'. Create one? (y/n): ",
                         session->username);
        return;

    case CONN_SET_PASSWORD:
        mud_telnet_echo_off(session->conn);
        mud_output_send(session->conn, "Choose new password: ");
        return;

    case CONN_CONFIRM_PASSWORD:
        mud_telnet_echo_off(session->conn);
        mud_output_send(session->conn, "Confirm password: ");
        return;

    case CONN_LIMBO:
        mud_telnet_echo_on(session->conn);
        mud_output_sendf(session->conn,
                         "\r\nWelcome, %s.\r\nType 'enter' or 'quit'.\r\n> ",
                         session->username);
        return;

    case CONN_PLAYING:
        mud_telnet_echo_on(session->conn);
        mud_output_send(session->conn, "\r\n> ");
        return;

    case CONN_DISCONNECTING:
        mud_telnet_echo_on(session->conn);
        mud_output_send_line(session->conn, "Connection closing. Goodbye.");
        mud_connection_close(session->conn);
        return;
    }
}

void mud_session_send(MudSession* session, const char* text) {
    if (!session_is_attached(session) || text == NULL) {
        return;
    }
    mud_output_send(session->conn, text);
}

void mud_session_sendf(MudSession* session, const char* fmt, ...) {
    char buffer[1024];
    va_list args;

    if (!session_is_attached(session) || fmt == NULL) {
        return;
    }

    va_start(args, fmt);
    (void)vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    mud_output_send(session->conn, buffer);
}

void mud_session_kick(MudSession* session, const char* reason) {
    const char* username = NULL;
    const char* why = reason != NULL ? reason : "No reason provided";

    if (!session_is_attached(session)) {
        return;
    }

    username = session->username[0] != '\0' ? session->username : "(unknown)";
    LOG_ADMIN_INFO("Kicking session username='%s' account=%" PRId64 " reason='%s'",
                   username,
                   session->account_id,
                   why);
    mud_output_sendf(session->conn, "Disconnected: %s\r\n", why);
    mud_session_transition(session, CONN_DISCONNECTING);
}
