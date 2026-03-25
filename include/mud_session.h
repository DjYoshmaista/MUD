#ifndef MUD_SESSION_H
#define MUD_SESSION_H

#include "mud_connection.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MUD_SESSION_MAX_CONNS_PER_USER    10
#define DEFAULT_USERNAME                  "Pierce"
#define DEFAULT_PASSWORD                  "lolcat"
#define charset                           "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

typedef enum SessionState {
    CONN_NEW = 0,
    CONN_GET_NAME,
    CONN_GET_PASSWORD,
    CONN_NEW_ACCOUNT,
    CONN_SET_PASS,
    CONN_CONFIRM_PASSWORD,
    CONN_LIMBO,
    CONN_PLAYING,
    CONN_DISC
} SessionState;

typedef struct MudSession {
    MudConnection* conn;
    SessionState state;
    int64_t account_id;
    char    username[64];
    char    pw_buf[256];
    int     failed_logins;
    int64_t connected_at;
    int64_t last_active_at;
    void*   player;
} MudSession;

// Create a random username
char* mud_make_dflt_username(char* out, size_t out_len);

// Create a random password
char* mud_make_dflt_password(char* out, size_t out_len);

/* Session Creation/Management */
// -- Session Creation --
MudSession* mud_session_create(MudConnection* conn);
// -- Session Destruction --
void mud_session_destroy(MudSession* session);
// -- Session Dispatch (main) --
void mud_session_on_line(MudSession* session, const char* line);
// -- Session State Management --
void mud_session_transition(MudSession* session, SessionState new_state);
// Convenience Wrapper
void mud_session_send(MudSession* session, const char* text);
// `printf`-style convenience wrapper
void mud_session_sendf(MudSession* session, const char* fmt, ...);
// Session Log + Close
void mud_session_kick(MudSession* session, const char* reason);

#ifdef __cplusplus
}
#endif

#endif // MUD_SESSION_H
