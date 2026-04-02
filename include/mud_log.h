#ifndef MUD_LOG_H
#define MUD_LOG_H

#include <zlog.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log lifecycle state
typedef enum g_log_state {
    G_LOG_STATE_UNINITIALIZED,
    G_LOG_STATE_INITIALIZED,
    G_LOG_STATE_SHUTDOWN
} g_log_state_t;

typedef g_log_state_t LogState;

/*  Initialize the logging system.  Must be called before any MUD_LOG_* macros are used.
    @param config_path  Path to zlog.conf.  If NULLL uses config/zlog.conf
    @return true on success, false if config file is missing or malformed
*/
bool mud_log_init(const char* config_path);

//  Flush all log outputs and shut down the logging system.  Call this during clean server shutdown
void mud_log_shutdown(void);
bool mud_log_reload(const char* config_path);

/*  Category handles - one per subsystem.
    These are opaque pointers to zlog category objects.  Never need to use them directly; the macros below do it for you.
    They are declared extern so every translation unit can see them after mud_log_init() has populated them.
*/
typedef zlog_category_t MudLogCategory;

extern MudLogCategory* g_log_core;
extern MudLogCategory* g_log_net;
extern MudLogCategory* g_log_session;
extern MudLogCategory* g_log_arena;
extern MudLogCategory* g_log_test;
extern MudLogCategory* g_log_world;
extern MudLogCategory* g_log_db;
extern MudLogCategory* g_log_script;
extern MudLogCategory* g_log_auth;
extern MudLogCategory* g_log_admin;

/*  Logging macros - use these everywhere in the codebase.

    Each subsystem has its own set.  The category is baked in at the call site so zlog can apply per-category rules from the config file.

    Usage:
        LOG_NET_INFO("Accepted connection from %s:%d", addr, port);
        LOG_NET_AUTH("Failed login attempt for account '%s'", username);
        LOG_WORLD_DEBUG("Player %u moved from room %u to room %u", pid, from, to);
*/

// Core / general
#define MUD_ZLOG_CALL(call, category, fmt, ...)                                    \
    do {                                                                            \
        if ((category) != NULL) {                                                   \
            call((category), fmt, ##__VA_ARGS__);                                   \
        }                                                                           \
    } while (0)

#define LOG_CORE_DEBUG(fmt, ...)    MUD_ZLOG_CALL(zlog_debug, g_log_core, fmt, ##__VA_ARGS__)
#define LOG_CORE_INFO(fmt, ...)     MUD_ZLOG_CALL(zlog_info,  g_log_core, fmt, ##__VA_ARGS__)
#define LOG_CORE_WARN(fmt, ...)     MUD_ZLOG_CALL(zlog_warn,  g_log_core, fmt, ##__VA_ARGS__)
#define LOG_CORE_ERROR(fmt, ...)    MUD_ZLOG_CALL(zlog_error, g_log_core, fmt, ##__VA_ARGS__)
#define LOG_CORE_FATAL(fmt, ...)    MUD_ZLOG_CALL(zlog_fatal, g_log_core, fmt, ##__VA_ARGS__)

// Test / general
#define LOG_TEST_DEBUG(fmt, ...)    MUD_ZLOG_CALL(zlog_debug, g_log_test, fmt, ##__VA_ARGS__)
#define LOG_TEST_INFO(fmt, ...)     MUD_ZLOG_CALL(zlog_info,  g_log_test, fmt, ##__VA_ARGS__)
#define LOG_TEST_WARN(fmt, ...)     MUD_ZLOG_CALL(zlog_warn,  g_log_test, fmt, ##__VA_ARGS__)
#define LOG_TEST_ERROR(fmt, ...)    MUD_ZLOG_CALL(zlog_error, g_log_test, fmt, ##__VA_ARGS__)
#define LOG_TEST_FATAL(fmt, ...)    MUD_ZLOG_CALL(zlog_fatal, g_log_test, fmt, ##__VA_ARGS__)
#define LOG_TEST_CHECKPOINT(fmt, ...)   MUD_ZLOG_CALL(zlog_warn, g_log_test, fmt, ##__VA_ARGS__)
#define LOG_TEST_ACTION(fmt, ...)    MUD_ZLOG_CALL(zlog_info,  g_log_test, fmt, ##__VA_ARGS__)

// Arena
#define LOG_ARENA_DEBUG(fmt, ...)   MUD_ZLOG_CALL(zlog_debug, g_log_arena, fmt, ##__VA_ARGS__)
#define LOG_ARENA_INFO(fmt, ...)    MUD_ZLOG_CALL(zlog_info,  g_log_arena, fmt, ##__VA_ARGS__)
#define LOG_ARENA_WARN(fmt, ...)    MUD_ZLOG_CALL(zlog_warn,  g_log_arena, fmt, ##__VA_ARGS__)
#define LOG_ARENA_ERROR(fmt, ...)   MUD_ZLOG_CALL(zlog_error, g_log_arena, fmt, ##__VA_ARGS__)
#define LOG_ARENA_FATAL(fmt, ...)   MUD_ZLOG_CALL(zlog_fatal, g_log_arena, fmt, ##__VA_ARGS__)

// Networking
#define LOG_NET_AUTH(fmt, ...)     MUD_ZLOG_CALL(zlog_info, g_log_net, fmt, ##__VA_ARGS__)
#define LOG_NET_DEBUG(fmt, ...)     MUD_ZLOG_CALL(zlog_debug, g_log_net, fmt, ##__VA_ARGS__)
#define LOG_NET_INFO(fmt, ...)      MUD_ZLOG_CALL(zlog_info,  g_log_net, fmt, ##__VA_ARGS__)
#define LOG_NET_WARN(fmt, ...)      MUD_ZLOG_CALL(zlog_warn,  g_log_net, fmt, ##__VA_ARGS__)
#define LOG_NET_ERROR(fmt, ...)     MUD_ZLOG_CALL(zlog_error, g_log_net, fmt, ##__VA_ARGS__)
#define LOG_NET_FATAL(fmt, ...)     MUD_ZLOG_CALL(zlog_fatal, g_log_net, fmt, ##__VA_ARGS__)

// Sessions
#define LOG_SESSION_DEBUG(fmt, ...) MUD_ZLOG_CALL(zlog_debug, g_log_session, fmt, ##__VA_ARGS__)
#define LOG_SESSION_INFO(fmt, ...)  MUD_ZLOG_CALL(zlog_info,  g_log_session, fmt, ##__VA_ARGS__)
#define LOG_SESSION_WARN(fmt, ...)  MUD_ZLOG_CALL(zlog_warn,  g_log_session, fmt, ##__VA_ARGS__)
#define LOG_SESSION_ERROR(fmt, ...) MUD_ZLOG_CALL(zlog_error, g_log_session, fmt, ##__VA_ARGS__)

// World Simulation
#define LOG_WORLD_DEBUG(fmt, ...)   MUD_ZLOG_CALL(zlog_debug, g_log_world, fmt, ##__VA_ARGS__)
#define LOG_WORLD_INFO(fmt, ...)    MUD_ZLOG_CALL(zlog_info,  g_log_world, fmt, ##__VA_ARGS__)
#define LOG_WORLD_WARN(fmt, ...)    MUD_ZLOG_CALL(zlog_warn,  g_log_world, fmt, ##__VA_ARGS__)
#define LOG_WORLD_ERROR(fmt, ...)   MUD_ZLOG_CALL(zlog_error, g_log_world, fmt, ##__VA_ARGS__)

// Database
#define LOG_DB_DEBUG(fmt, ...)      MUD_ZLOG_CALL(zlog_debug, g_log_db, fmt, ##__VA_ARGS__)
#define LOG_DB_INFO(fmt, ...)       MUD_ZLOG_CALL(zlog_info,  g_log_db, fmt, ##__VA_ARGS__)
#define LOG_DB_WARN(fmt, ...)       MUD_ZLOG_CALL(zlog_warn,  g_log_db, fmt, ##__VA_ARGS__)
#define LOG_DB_ERROR(fmt, ...)      MUD_ZLOG_CALL(zlog_error, g_log_db, fmt, ##__VA_ARGS__)

// Scripting (Lua)
#define LOG_SCRIPT_DEBUG(fmt, ...)  MUD_ZLOG_CALL(zlog_debug, g_log_script, fmt, ##__VA_ARGS__)
#define LOG_SCRIPT_INFO(fmt, ...)   MUD_ZLOG_CALL(zlog_info,  g_log_script, fmt, ##__VA_ARGS__)
#define LOG_SCRIPT_WARN(fmt, ...)   MUD_ZLOG_CALL(zlog_warn,  g_log_script, fmt, ##__VA_ARGS__)
#define LOG_SCRIPT_ERROR(fmt, ...)  MUD_ZLOG_CALL(zlog_error, g_log_script, fmt, ##__VA_ARGS__)

// Authentication
#define LOG_AUTH_SUCCESS(fmt, ...)    MUD_ZLOG_CALL(zlog_debug, g_log_auth, fmt, ##__VA_ARGS__)
#define LOG_AUTH_INFO(fmt, ...)     MUD_ZLOG_CALL(zlog_info,  g_log_auth, fmt, ##__VA_ARGS__)
#define LOG_AUTH_WARN(fmt, ...)     MUD_ZLOG_CALL(zlog_warn,  g_log_auth, fmt, ##__VA_ARGS__)
#define LOG_AUTH_ERROR(fmt, ...)    MUD_ZLOG_CALL(zlog_error, g_log_auth, fmt, ##__VA_ARGS__)
#define LOG_AUTH_FAIL(fmt, ...)     MUD_ZLOG_CALL(zlog_fatal, g_log_auth, fmt, ##__VA_ARGS__)

// Administration
#define LOG_ADMIN_INFO(fmt, ...)    MUD_ZLOG_CALL(zlog_info,  g_log_admin, fmt, ##__VA_ARGS__)
#define LOG_ADMIN_WARN(fmt, ...)    MUD_ZLOG_CALL(zlog_warn,  g_log_admin, fmt, ##__VA_ARGS__)
#define LOG_ADMIN_ERROR(fmt, ...)   MUD_ZLOG_CALL(zlog_error, g_log_admin, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // MUD_LOG_H
