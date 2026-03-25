#include "mud_db.h"
#include "mud_session.h"
#include "mud_connection.h"
#include "mud_log.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>

MudSession* mud_session_create(MudConnection* conn) {
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
    while (new_state <= CONN_DISCONNECTING) {
        switch (new_state) {
            case CONN_NEW:
                LOG_SESSION_INFO("SessionState is CONN_NEW; please update SessionState and initialize MudSession");
                new_sess->failed_logins = 0;
                new_sess->connected_at = time(NULL);
                new_sess->last_active_at = time(NULL);
                new_sess->player = NULL;
                LOG_SESSION_INFO("SessionState Updated to CONN_GET_NAME");
                new_state = CONN_GET_NAME;
                continue;
            case CONN_GET_NAME:
                LOG_SESSION_DEBUG("SessionState is CONN_GET_NAME; please set username: ");
                scanf("%s", line);
                if (mud_session_on_line(session->username, line) {
                    LOG_SESSION_INFO("Username input successful!  Please input password: ");
                    new_state = CONN_GET_PASSWORD;
                    continue;
                } else {
                    LOG_SESSION_INFO("Username input failed!  Please try again.");
                    new_state = CONN_GET_NAME;
                    continue;
                }
                break;
            case CONN_GET_PASSWORD:
                LOG_SESSION_DEBUG("SessionState is CONN_GET_PASSWORD; please set password");
                break;
            case CONN_NEW_ACCOUNT:
                LOG_SESSION_DEBUG("SessionState is CONN_NEW_ACCOUNT; please set account_id");
                break;
            case CONN_SET_PASSWORD:
                LOG_SESSION_DEBUG("SessionState is CONN_SET_PASSWORD; setting password");
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

void mud_session_send(MudSession* session, const char* text) {
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
    LOG_ADMIN_INFO("Kicking player '%s' from session '%llu'.  Reason: '%s'", session->player->username, (unsigned long long)session->id, reason);q
}
