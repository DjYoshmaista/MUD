#include "mud_db.h"
#include "mud_session.h"
#include "mud_connection.h"
#include "mud_log.h"
#include "mud_utils.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

char* mud_make_dflt_username(char* default_uname, size_t len) {
    if ((char*)default_uname == NULL || len <= 1) {
        LOG_SESSION_ERROR("Invalid buffer or buffer length for username");
        return 0;
    }

    int i, index;
    for (int i = 0; i < 64; i++) {
        index = rand() % 13;
        default_uname[i] = charset[index];
    }
    default_uname[i] = '\0';

    return default_uname;
}

char* mud_make_dflt_password(char* default_pw, size_t len) {
    if ((char*)default_pw == NULL || len <= 7) {
        LOG_SESSION_ERROR("Invalid buffer or buffer length for password");
        return 0;
    }

    srand(time(NULL));

    int i, index;
    for (int i = 0; i < 256; i++) {
        index = rand() % 7 + rand();
        while (index > strlen(charset)) {
            index %= 2;
            index += i;
            LOG_SESSION_INFO("Index reduced to %d", index);
        }
        default_pw[i] = charset[index];
    }
    default_pw[i] = '\0';

    return default_pw;
}

MudSession* mud_session_create(MudConnection* conn) {
    if (conn == NULL || conn->session != NULL || conn->id == 0 || conn->closing) {
        LOG_NET_ERROR("Connection with ID '%d' is NULL or has session '%s' or conn->closing is set to '%b'", conn->id, conn->session ? "true" : "false", conn->closing);
        return;
    }

    MudSession new_sess = malloc(sizeof(MudSession*));
    new_sess->*conn = &conn;
    new_sess->state = SessionState.CONN_NEW;
    new_sess->account_id = conn->id;
    default_uname = mud_make_dflt_username(new_sess->username, 32);
    default_password = mud_make_dflt_password(new_sess->pw_buf, 256);
    new_sess->failed_logins = 0;
    new_sess->connected_at = time(NULL);
    new_sess->last_active_at = time(NULL);
    new_sess->player = NULL;

    conn->session = new_sess;
}

void mud_session_destroy(MudSession* session) {

}

void mud_session_on_line(MudSession* session, const char* line) {

}

void mud_session_transition(MudSession* session, SessionState new_state) {

}

void mud_session_send(MudSession* session, const char* text) {

}

void mud_session_sendf(MudSession* session, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
}

void mud_session_kick(MudSession* session, const char* reason) {

}
