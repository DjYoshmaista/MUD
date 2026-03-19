#include "mud_config.h"
#include "mud_crypto.h"
#include "mud_db.h"
#include "mud_log.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void shutdown_subsystems(bool config_loaded, bool db_open) {
    if (db_open) {
        mud_db_close();
    }
    if (config_loaded) {
        mud_config_shutdown();
    }
    mud_log_shutdown();
}

int main(void) {
    bool config_loaded = false;
    bool db_open = false;
    const char* log_config_path = "config/zlog.conf";
    const char* db_path = NULL;

    if (!mud_log_init(log_config_path)) {
        fprintf(stderr, "Failed to initialize logging using '%s'\n", log_config_path);
        return EXIT_FAILURE;
    }

    if (!mud_config_load(NULL)) {
        LOG_CORE_FATAL("Failed to load configuration");
        shutdown_subsystems(false, false);
        return EXIT_FAILURE;
    }
    config_loaded = true;

    if (!mud_crypto_init()) {
        LOG_CORE_FATAL("Failed to initialize crypto");
        shutdown_subsystems(config_loaded, false);
        return EXIT_FAILURE;
    }

    db_path = mud_config_get_string("database.path", "data/mud.db");
    if (!mud_db_open(db_path)) {
        LOG_CORE_FATAL("Failed to open database '%s'", db_path);
        shutdown_subsystems(config_loaded, false);
        return EXIT_FAILURE;
    }
    db_open = true;

    if (!mud_db_migrate()) {
        LOG_CORE_FATAL("Failed to migrate database schema");
        shutdown_subsystems(config_loaded, db_open);
        return EXIT_FAILURE;
    }

    LOG_CORE_INFO("Startup checks completed successfully");
    shutdown_subsystems(config_loaded, db_open);
    return EXIT_SUCCESS;
}
