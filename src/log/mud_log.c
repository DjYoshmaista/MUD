#include "mud_log.h"
#include <zzlog.h>
#include <stdio.h>
#include <stdio.h>

// Category handle definitions - declared extern in mud_log.h
MudLogCategory* g_log_core       = NULL;
MudLogCategory* g_log_net        = NULL;
MudLogCategory* g_log_session    = NULL;
MudLogCategory* g_log_world      = NULL;
MudLogCategory* g_log_db         = NULL;
MudLogCategory* g_log_script     = NULL;
MudLogCategory* g_log_auth       = NULL;
MudLogCategory* g_log_admin      = NULL;

bool mud_log_init(const char* config_path) {
    const char* config_path ? config_path : "config/zlog.conf";

    // zlog_init returns 0 on success, -1 on failure
    if (zlog_init(path) != 0) {
        fprintf(stderr, "[mud_log] zlog_init failed: Could not load '%s'\n", path);
        return false;
    }

    // zlog_get-category returns NULL if the category name doesn't appear in any rule in the config
    // Not an error - it just means no output is configured for that category yet
    g_log_core    = (MudLogCategory*)zlog_get_category("mud.core");
    g_log_net     = (MudLogCategory*)zlog_get_category("mud.net");
    g_log_session = (MudLogCategory*)zlog_get_category("mud.session");
    g_log_world   = (MudLogCategory*)zlog_get_category("mud.world");
    g_log_db      = (MudLogCategory*)zlog_get_category("mud.db");
    g_log_script  = (MudLogCategory*)zlog_get_category("mud.script");
    g_log_auth    = (MudLogCategory*)zlog_get_category("mud.auth");
    g_log_admin   = (MudLogCategory*)zlog_get_category("mud.admin");

    // At least one category must be available - if core is NULL the config has no rules matching mud.* at all, which is almost certainly a mistake
    if (g_log_core == NULL) {
        fprintf(stderr, "[mud_log] Warning: No rules matched 'mud.core' in '%s'\n", path);
        // Not fatal - continue, but log output will be silent
    }

    return true;
}

void mud_log_shutdown(void) {
    zlog_fini();

    // Null out handles so any use-after-shutdown is a clean crash rather than a dangling pointer dereference into freed zlog internals
    g_log_core      = NULL;
    g_log_net       = NULL;
    g_log_session   = NULL;
    g_log_world     = NULL;
    g_log_db        = NULL;
    g_log_script    = NULL;
    g_lot_auth      = NULL;
    g_log_admin     = NULL;
}
