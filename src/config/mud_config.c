#include "mud_config.h"
#include "mud_log.h"
#include "mud_db.h"

#include <libconfig.h>
#include <stdio.h>

static config_t g_config;
static bool     g_loaded = false;

bool mud_config_load(const char* path) {
    const char* p = path ? path : "config/mud.conf";

    if (g_loaded) {
        LOG_SESSION_DEBUG("Config already loaded");
        config_destroy(&g_config);
        g_loaded = false;
    }

    LOG_SESSION_DEBUG("Loading config from '%s'", p);
    config_init(&g_config);

    if (config_read_file(&g_config, p) != CONFIG_TRUE) {
        fprintf(stderr, "Config error in '%s' at line %d: %s\n",
                p, config_error_line(&g_config), config_error_text(&g_config));
        config_destroy(&g_config);
        return false;
    }
    LOG_SESSION_DEBUG("Config loaded successfully");
    g_loaded = true;
    return true;
}

void mud_config_shutdown(void) {
    if (g_loaded) {
        config_destroy(&g_config);
        g_loaded = false;
    }
}

const char* mud_config_get_string(const char* path, const char* def) {
    if (!g_loaded || !path) {
        if (!g_loaded) {
            LOG_SESSION_DEBUG("Config not loaded");
        } else if (!path) LOG_SESSION_DEBUG("Path is NULL");
        fprintf(stderr, "[mud_config_get_string]::Config error: path is null or config not loaded\n");
        return def;
    }
    const char* val;
    return config_lookup_string(&g_config, path, &val) == CONFIG_TRUE ? val : def;
}

int mud_config_get_int(const char* path, int def) {
    if (!g_loaded || !path) {
        if (!g_loaded) {
            LOG_SESSION_DEBUG("Config not loaded");
        } else if (!path) LOG_SESSION_DEBUG("Path is NULL");
        fprintf(stderr, "[mud_config_get_int]::Config error: path is null or config not loaded\n");
        return def;
    }
    int val;
    return config_lookup_int(&g_config, path, &val) == CONFIG_TRUE ? val : def;
}

bool mud_config_get_bool(const char* path, bool def) {
    if (!g_loaded || !path) {
        if (!g_loaded) {
            LOG_SESSION_DEBUG("Config not loaded");
        } else if (!path) LOG_SESSION_DEBUG("Path is NULL");
        fprintf(stderr, "[mud_config_get_bool]::Config error: path is null or config not loaded\n");
        return def;
    }
    int val;
    return config_lookup_bool(&g_config, path, &val) == CONFIG_TRUE ? (val != 0) : def;
}

double mud_config_get_double(const char* path, double def) {
    if (!g_loaded || !path) {
        if (!g_loaded) {
            LOG_SESSION_DEBUG("Config not loaded");
        } else if (!path) LOG_SESSION_DEBUG("Path is NULL");
        fprintf(stderr, "[mud_config_get_double]::Config error: path is null or config not loaded\n");
        return def;
    }
    double val;
    return config_lookup_float(&g_config, path, &val) == CONFIG_TRUE ? val : def;
}
