#include "mud_log.h"
#include <zlog.h>
#include <stdio.h>
#include <string.h>

// Catgeory handle definitions -declared extern in mud_log.h, defined once here
MudLogCategory* g_log_core          = NULL;
MudLogCategory* g_log_net           = NULL;
MudLogCategory* g_log_session       = NULL;
MudLogCategory* g_log_world         = NULL;
MudLogCategory* g_log_db            = NULL;
MudLogCategory* g_log_script        = NULL;
MudLogCategory* g_log_auth          = NULL;
MudLogCategory* g_log_admin         = NULL;

bool mud_log_init(const char* config_path) {
    const char* path = config_path ? config_path : "config/zlog.conf";

    // zlog_init returns 0 on success, -1 on failure
    if (zlog_init(path) != 0) {
        fprintf(stderr, "[mud_log] zlog_init failed: could not load '%s'\n", path);
        return false;
    }

    // zlog_get_category returns NULL if the ccategory name doesn't appear in any rule in the config.
    // Not an error - just means no output is configured for that category yet
    g_log_core        = (MudLogCategory*)zlog_get_category("core");
    g_log_net         = (MudLogCategory*)zlog_get_category("net");
    g_log_session     = (MudLogCategory*)zlog_get_category("session");
    g_log_world       = (MudLogCategory*)zlog_get_category("world");
    g_log_db          = (MudLogCategory*)zlog_get_category("db");
    g_log_script      = (MudLogCategory*)zlog_get_category("script");
    g_log_auth        = (MudLogCategory*)zlog_get_category("auth");
    g_log_admin       = (MudLogCategory*)zlog_get_category("admin");

    // At least one category must be available - if core is NULL, config has no rules matching mud.* at all
    if (g_log_core == NULL) {
        fprintf(stderr, "[mud_log] Warning: no rules matched 'mud.core' in '%s'\n", path);
        // Not fatal; continue (logging silenced)
    }

    return true;
}

void mud_log_shutdown(void) {
    zlog_fini();

    // Null out handles so any use-after-shutdown is a clean crash rather than a dangling pointer dereference into freed zlog internals
    g_log_core        = NULL;
    g_log_net         = NULL;
    g_log_session     = NULL;
    g_log_world       = NULL;
    g_log_db          = NULL;
    g_log_script      = NULL;
    g_log_auth        = NULL;
    g_log_admin       = NULL;
}
