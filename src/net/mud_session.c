#include "mud_db.h"
#include "mud_session.h"
#include "mud_connection.h"
#include "mud_log.h"

#include <sqlite3.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>

bool* g_log_initialized;

static MudDbAccount* new_account;

static bool ensure_log_initialized(void) {
    if (!g_log_initialized) {
        mud_log_init("logs/mud_session.log");
        g_log_initialized = true;
    }
    return true;
}

static MudDbAccount* session_lookup_account(MudSession* session) {
    g_log_initialized = ensure_log_initialized();
    if (session == NULL) {
        LOG_SESSION_WARN("[session_lookup_account] Session is NULL");
        return NULL;
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("[session_lookup_account] Session->Connection is NULL");
        return NULL;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("[session_lookup_account] Session->Connection->Session is not the same as the session");
        return NULL;
    }
    MudDbAccount* account = NULL;
    if (!mud_db_account_get_by_name(session->username, (MudDbAccount*)account)) {
        LOG_SESSION_WARN("[session_lookup_account] Failed to lookup account");
        return NULL;
    }
    return account;
}

static bool session_authenticate_password(MudSession* session, MudDbAccount* account) {
    g_log_initialized = ensure_log_initialized();
    if (session == NULL) {
        LOG_SESSION_WARN("[session_authenticate_password] Session is NULL");
        return false;
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("[session_authenticate_password] Session->Connection is NULL");
        return false;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("[session_authenticate_password] Session->Connection->Session is not the same as the session");
        return false;
    }
    if (account == NULL) {
        LOG_SESSION_WARN("[session_authenticate_password] Account is NULL");
        return false;
    }
    LOG_AUTH_INFO("Please enter password for account '%s': ", session->username);
    char* pinput;
    scanf("%s", pinput);
    const char* pline = pinput;
    if (mud_session_on_line(session->pw_buf, pline)) {
}

MudSession* mud_session_create(MudConnection* conn) {
    g_log_initialized = ensure_log_initialized();
    if (!*g_log_initialized) {
        mud_log_init(NULL);
        *g_log_initialized = true;
    }
    if (conn == NULL) {
        LOG_NET_ERROR("Connection is NULL");
        return NULL;
    }
    if (conn->session != NULL || conn->id == 0 || conn->closing) {
        const char* conn_closing = conn->closing ? "true" : "false";
        LOG_NET_ERROR("Connection with ID '''%llu''' has '0' MudConnection ID '''%llu''', or conn->closing is set to '''%s'''",
                (unsigned long long)conn->id, (unsigned long long)conn->id, conn_closing);
        return NULL;
    }

    MudSession* new_sess = (MudSession*)malloc(sizeof(MudSession));
    if (new_sess == NULL) {
        LOG_NET_ERROR("Failed to allocate memory for new session");
        return NULL;
    }
    memset(new_sess, 0, sizeof(MudSession));

    new_sess->conn = (MudConnection*)conn;
    new_sess->state = CONN_NEW; 
    mud_session_transition(new_sess, CONN_NEW);

    conn->session = (MudSession*)new_sess;
    LOG_SESSION_INFO("***NEW SESSION***:\nSession->Connection ID: '%llu'::Session->Username '%s'::Sess->Account_ID `%llu'", (unsigned long long)conn->id, new_sess->username, (unsigned long long)new_sess->account_id);
    LOG_SESSION_INFO("***STATS***\nFailed Logins: '%d'::Connected At: '" PRId64 "'::Last Active At: '" PRId64 "'", new_sess->failed_logins, new_sess->connected_at, new_sess->last_active_at);
    return new_sess;
}

void mud_session_destroy(MudSession* session) {
    g_log_initialized = ensure_log_initialized();
    if (session == NULL) {
        LOG_SESSION_WARN("Session is NULL");
        return;
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("Session->Connection is NULL");
        return;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("Session->Connection->Session is not the same as the session");
        return;
    }
    session->conn->session = NULL;
    free(session);
}

void mud_session_on_line(MudSession* session, const char* line) {
    g_log_initialized = ensure_log_initialized();
    if (session == NULL) {
        LOG_SESSION_WARN("Session is NULL");
        return;
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("Session->Connection is NULL");
        return;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("Session->Connection->Session is not the same as the session");
        return;
    }

    LOG_SESSION_INFO("Session->Connection->Session->Line: '%s'", line);
    // TODO: Implement this
}

void mud_session_transition(MudSession* session, SessionState new_state) {
    g_log_initialized = ensure_log_initialized();
    if (session == NULL) {
        LOG_SESSION_WARN("Session is NULL");
        return;
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("Session->Connection is NULL");
        return;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("Session->Connection->Session is not the same as the session");
        return;
    }
    if (session->state == new_state) {
        LOG_SESSION_WARN("Session->State is already '%d'", new_state);
        return;
    }
    char* line;
    session->state = new_state;
    LOG_SESSION_INFO("Session->State is now '%d'", new_state);
    session->failed_logins = 0;
    while (session->state <= CONN_DISCONNECTING) {
        switch (new_state) {
            case CONN_NEW:
                // Connection accepted; negotiate telnet
                LOG_SESSION_INFO("SessionState is CONN_NEW; please update SessionState and initialize MudSession");
                if (mud_telnet_attach(session->conn)) {
                    LOG_SESSION_INFO("Telnet negotiation successful.  SessionState incrementing from CONN_NEW to CONN_GET_NAME");
                    session->state++;
                    continue;
                } else {
                    LOG_SESSION_INFO("Telnet negotiation failed.  Retrying in 5 seconds...");
                    sleep(5);
                    // Increment failed logins
                    session->failed_logins++;
                    // TODO: Placeholder for telnet negotiation failure
                    if (session->failed_logins > 5) {
                        LOG_SESSION_INFO("Failed to negotiate telnet.  Disconnecting...");
                        session->state = CONN_DISCONNECTING;
                    }
                    continue;
                }
            case CONN_GET_NAME:
                // Telnet negotiation successful; get username
                LOG_SESSION_DEBUG("SessionState is CONN_GET_NAME; please set username: ");
                // Get username from user via telnet session
                if (session_set_username(char* username, MudDbAccount* account)) {
                    LOG_SESSION_INFO("Username input successful!");
                    session->state++;
                    continue;
                } else {
                    LOG_SESSION_INFO("Username not found in database.  Enter '1' to create new account, '2' to retry input, or '3' to exit");
                    switch (mud_

                break;
            case CONN_GET_PASSWORD:
                LOG_SESSION_DEBUG("SessionState is CONN_GET_PASSWORD; please set password");
                break;
            case CONN_NEW_ACCOUNT:
                LOG_SESSION_DEBUG("SessionState is CONN_NEW_ACCOUNT; Generating account ID");
                const char* line;
                MudDbAccount* account
                if (mud_db_account_get_by_name(session->username,   {
                    LOG_SESSION_INFO("Username input successful!  Please input password: ");
                    session->new_state++;
                    continue;
                } else {
                    LOG_SESSION_INFO("Username input failed!  Please try again.");
                    continue;
                }
                break;
            case CONN_SET_PASSWORD:
                char* pinput;
                LOG_SESSION_DEBUG("SessionState is CONN_SET_PASSWORD; setting password");
                scanf("%s", pinput);
                const char* pline = pinput;
                if (mud_session_on_line(session->pw_buf, pline)) {
                    LOG_SESSION_INFO("Please confirm password: ");
                    char* pinput2;
                    scanf("%s", pinput2);
                    if (strcmp(pinput, pinput2) == 0) {
                        LOG_SESSION_INFO("Passwords match!  Hashing password...");
                        // TODO: Hash password
                    } else {
                        LOG_SESSION_INFO("Passwords do not match!  Please try again.");
                        continue;
                    }
                    session->new_state++;
                }
                break;
            case CONN_CONFIRM_PASSWORD:
                LOG_SESSION_DEBUG("SessionState is CONN_CONFIRM_PASSWORD; confirming password");
                break;
            case CONN_LIMBO:
                LOG_SESSION_DEBUG("SessionState is CONN_LIMBO; please update SessionState and verify connection");
                break;
            case CONN_PLAYING:
                LOG_SESSION_DEBUG("SessionState is CONN_PLAYING; Entering world...");
                break;
            case CONN_DISCONNECTING:
                LOG_SESSION_DEBUG("SessionState is CONN_DISCONNECTING; disconnecting...");
                break;
            default:
                LOG_SESSION_WARN("SessionState is unknown; please update SessionState and verify connection");
                break;
        }
    }
}

void mud_session_send(MudSession* session, const char* text) {
    g_log_initialized = ensure_log_initialized();
    if (session == NULL) {
        LOG_SESSION_WARN("Session is NULL");
        return;   
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("Session->Connection is NULL");
        return;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("Session->Connection->Session is not the same as the session");
        return;
    }
    if (session->state != CONN_PLAYING) {
        LOG_SESSION_WARN("Session->State is not CONN_PLAYING");
        return;
    }
}

void mud_session_sendf(MudSession* session, const char* fmt, ...) {
    g_log_initialized = ensure_log_initialized();
    va_list args;
    va_start(args, fmt);
    if (session == NULL) {
        LOG_SESSION_WARN("Session is NULL");
        return;   
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("Session->Connection is NULL");
        return;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("Session->Connection->Session is not the same as the session");
        return;
    }
    if (session->state != CONN_PLAYING) {
        LOG_SESSION_WARN("Session->State is not CONN_PLAYING");
        return;
    }
}

void mud_session_kick(MudSession* session, const char* reason) {
    g_log_initialized = ensure_log_initialized();
    if (session == NULL) {
        LOG_SESSION_WARN("Session is NULL");
        return;   
    }
    if (session->conn == NULL) {
        LOG_SESSION_WARN("Session->Connection is NULL");
        return;
    }
    if (session->conn->session != session) {
        LOG_SESSION_WARN("Session->Connection->Session is not the same as the session");
        return;
    }
    if (session->state != CONN_PLAYING) {
        LOG_SESSION_WARN("Session->State is not CONN_PLAYING");
        return;
    }
    LOG_ADMIN_INFO("Kicking player '%s' from session '%llu'.  Reason: '%s'", session->player->username, (unsigned long long)session->id, reason);
}
