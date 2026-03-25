#include "mud_config.h"
#include "mud_connection.h"
#include "mud_listener.h"
#include "mud_crypto.h"
#include "mud_db.h"
#include "mud_log.h"
#include "mud_net_loop.h"

#include <stdbool.h>
#include <stdlib.h>

static void shutdown_subsystems(bool db_loaded, bool config_loaded, bool log_loaded) {
    mud_listener_stop();
    mud_connection_table_shutdown();
    mud_net_loop_close();

    if (db_loaded) {
        mud_db_close();
    }
    if (config_loaded) {
        mud_config_shutdown();
    }
    if (log_loaded) {
        mud_log_shutdown();
    }
}

int main(void) {
    bool log_loaded = false;
    bool config_loaded = false;
    bool db_loaded = false;
    int port = 4000;
    int max_connections = 256;

    if (!mud_log_init(MUD_ZLOG_PATH)) {
        return EXIT_FAILURE;
    }
    log_loaded = true;

    if (!mud_config_load(MUD_CONFIG_PATH)) {
        LOG_CORE_FATAL("Failed to load config from '%s'", MUD_CONFIG_PATH);
        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
        return EXIT_FAILURE;
    }
    config_loaded = true;

    if (!mud_crypto_init()) {
        LOG_CORE_FATAL("Failed to initialize crypto subsystem");
        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
        return EXIT_FAILURE;
    }

    if (!mud_db_open(MUD_DB_PATH)) {
        LOG_CORE_FATAL("Failed to open database at '%s'", MUD_DB_PATH);
        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
        return EXIT_FAILURE;
    }
    db_loaded = true;

    if (!mud_db_migrate()) {
        LOG_CORE_FATAL("Failed to migrate database schema");
        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
        return EXIT_FAILURE;
    }

    port = mud_config_get_int("network.telnet.port", 4000);
    max_connections = mud_config_get_int("network.max_connections", 256);

    mud_connection_table_init();

    if (!mud_net_loop_init()) {
        LOG_CORE_FATAL("Failed to initialize network subsystem");
        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
        return EXIT_FAILURE;
    }

    if (!mud_listener_start(port, max_connections)) {
        LOG_CORE_FATAL("Failed to start listener");
        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
        return EXIT_FAILURE;
    }

    LOG_CORE_INFO("MUD server starting on port %d", port);
    mud_net_loop_run();
    LOG_CORE_INFO("MUD server shutting down");

    LOG_CORE_INFO("Startup checks completed successfully");

    // Cleanup
    LOG_CORE_INFO("Shutting down MUD server");
    shutdown_subsystems(db_loaded, config_loaded, log_loaded);
    return EXIT_SUCCESS;
}
