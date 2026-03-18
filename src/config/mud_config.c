#include "mud_config.h"
#include "mud_log.h"
#include "mud_db."

#include <libconfig.h>
#include <stdio.h>

static config_t g_config;
static bool     g_loaded = false;

bool mud_config_load(const char* path) {
    const char* p = path ? path : "config/mud.conf";
    LOG_SESSION_TRACE("Loading config from '%s'", path);
    config_init(&g_config);

    if (config_read_file(&g_config, p) != CONFIG_TRUE) {
        fprintf(stderr, "Config error in '%s' at line %d: %s\n",
                p, config_error_line(&g_config), config_error_text(&g_config));
        config_destroy(&g_config);
        return false;
    }
    LOG_SESSION_TRACE("Config loaded successfully");
    g_loaded = true;
    return true;
}

void mud_config_shutdown(void) {
    LOG_SESSION_TRACE("Shutting down config");
    if (g_loaded) {
        config_destroy(&g_config);
        g_loaded = false;
    }
    LOG_SESSION_TRACE("Config destroyed and shutdown");
}

const char* mud_config_get_string(const char* path, const char* def) {
    if (!g_loaded || !path) {
        LOG_SESSION_TRACE("Config not loaded, returning default");
        return def;
    }
    LOG_SESSION_TRACE("Looking up '%s'", path);
    const char* val;
    return config_lookup_string(&g_config, path, &val) == CONFIG_TRUE ? val : def;
}

int mud_config_get_int(const char* path, int def) {
    if (!g_loaded || !path) {
        LOG_SESSION_TRACE("Config not loaded, returning default");
        return def;
    }
    LOG_SESSION_TRACE("Looking up value for '%d' in configuration file located at '%s'", def, path);
    int val;
    return config_lookup_int(&g_config, path, &val) == CONFIG_TRUE ? val : def;
}

bool mud_config_get_bool(const char* path, bool def) {
    if (!g_loaded || !path) {
        LOG_SESSION_TRACE("Config not loaded, returning default");
        return def;
    }
    LOG_SESSION_TRACE("Looking up value for '%d' in configuration file located at '%s'", def, path);
    int val;
    return config_lookup_bool(&g_config, path, &val) == CONFIG_TRUE ? (val != 0) : def;
}

double mud_config_get_double(const char* path, double def) {
    if (!g_loaded || !path) {
        LOG_SESSION_TRACE("Config not loaded, returning default");
        return def;
    }
    LOG_SESSION_TRACE("Looking up value for '%f' in configuration file located at '%s'", def, path);
    double val;
    return config_lookup_float(&g_config, path, &val) == CONFIG_TRUE ? val : def;
}
