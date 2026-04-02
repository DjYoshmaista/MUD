#include "mud_log.h"

#include <stdio.h>

MudLogCategory* g_log_core = NULL;
MudLogCategory* g_log_net = NULL;
MudLogCategory* g_log_session = NULL;
MudLogCategory* g_log_arena = NULL;
MudLogCategory* g_log_test = NULL;
MudLogCategory* g_log_world = NULL;
MudLogCategory* g_log_db = NULL;
MudLogCategory* g_log_script = NULL;
MudLogCategory* g_log_auth = NULL;
MudLogCategory* g_log_admin = NULL;

static LogState g_log_state = G_LOG_STATE_UNINITIALIZED;

static void mud_log_clear_categories(void) {
    g_log_core = NULL;
    g_log_net = NULL;
    g_log_session = NULL;
    g_log_arena = NULL;
    g_log_test = NULL;
    g_log_world = NULL;
    g_log_db = NULL;
    g_log_script = NULL;
    g_log_auth = NULL;
    g_log_admin = NULL;
}

static bool mud_log_bind_categories(void) {
    g_log_core = zlog_get_category("mud.core");
    g_log_net = zlog_get_category("mud.net");
    g_log_session = zlog_get_category("mud.session");
    g_log_arena = zlog_get_category("mud.arena");
    g_log_test = zlog_get_category("mud.test");
    g_log_world = zlog_get_category("mud.world");
    g_log_db = zlog_get_category("mud.db");
    g_log_script = zlog_get_category("mud.script");
    g_log_auth = zlog_get_category("mud.auth");
    g_log_admin = zlog_get_category("mud.admin");

    return g_log_core != NULL &&
           g_log_net != NULL &&
           g_log_session != NULL &&
           g_log_arena != NULL &&
           g_log_test != NULL &&
           g_log_world != NULL &&
           g_log_db != NULL &&
           g_log_script != NULL &&
           g_log_auth != NULL &&
           g_log_admin != NULL;
}

bool mud_log_init(const char* config_path) {
    const char* path = config_path != NULL ? config_path : "config/zlog.conf";

    if (g_log_state == G_LOG_STATE_INITIALIZED) {
        return true;
    }

    if (zlog_init(path) != 0) {
        fprintf(stderr, "[mud_log] zlog_init failed: could not load '%s'\n", path);
        return false;
    }

    if (!mud_log_bind_categories()) {
        fprintf(stderr, "[mud_log] failed to bind one or more log categories\n");
        zlog_fini();
        mud_log_clear_categories();
        return false;
    }

    g_log_state = G_LOG_STATE_INITIALIZED;
    return true;
}

bool mud_log_reload(const char* config_path) {
    const char* path = config_path != NULL ? config_path : "config/zlog.conf";

    if (g_log_state != G_LOG_STATE_INITIALIZED) {
        return false;
    }

    if (zlog_reload(path) != 0) {
        return false;
    }

    return mud_log_bind_categories();
}

void mud_log_shutdown(void) {
    if (g_log_state != G_LOG_STATE_INITIALIZED) {
        return;
    }

    zlog_fini();
    mud_log_clear_categories();
    g_log_state = G_LOG_STATE_SHUTDOWN;
}
