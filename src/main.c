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
    bool log_loaded = false, config_loaded = false, db_loaded = false, crypto_init = false, db_migrated = false, net_loop_init = false, listener_started = false;

    if (!config_loaded) {
        for (int i = 0; i < 5; i++) {
            if (!config_loaded) {
                config_loaded = mud_config_load("config/mud.conf");
                if (config_loaded) {
                    LOG_CORE_INFO("Config loaded successfully");
                    break;
                } else {
                    LOG_CORE_ERROR("Failed to load config from '%s'", MUD_CONFIG_PATH);
                    if (i == 4) {
                        LOG_CORE_FATAL("Failed to load config from '%s'", MUD_CONFIG_PATH);
                        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
                        return EXIT_FAILURE;
                    }
                }
            }
        }
    }

    int port = mud_config_get_int("network.telnet.port", 4000);
    int max_connections = mud_config_get_int("network.max_connections", 256);

    if (!log_loaded) {
        for (int i = 0; i < 5; i++) {
            if (!log_loaded) {
                log_loaded = mud_log_init("config/zlog.conf");
                if (log_loaded) {
                    LOG_CORE_INFO("Logging initialized successfully");
                    break;
                } else {
                    LOG_CORE_ERROR("Failed to initialize logging");
                    if (i == 4) {
                        LOG_CORE_FATAL("Failed to initialize logging");
                        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
                        return EXIT_FAILURE;
                    }
                }
            }
        }
    }

    if (!crypto_init) {
        for (int i = 0; i < 5; i++) {
            crypto_init = mud_crypto_init();
            if (crypto_init) {
                LOG_CORE_INFO("Crypto initialized successfully");
                break;
            } else {
                LOG_CORE_ERROR("Failed to initialize crypto");
                if (i == 4) {
                    LOG_CORE_FATAL("Failed to initialize crypto");
                    shutdown_subsystems(db_loaded, config_loaded, log_loaded);
                    return EXIT_FAILURE;
                }
            }
        }
    }

    if (!db_loaded) {
        for (int i = 0; i < 5; i++) {
            if (!db_loaded) {
                db_loaded = mud_db_open(MUD_DB_PATH);
                if (db_loaded) {
                    LOG_CORE_INFO("Database initialized successfully");
                    break;
                } else {
                    LOG_CORE_ERROR("Failed to initialize database");
                    if (i == 4) {
                        LOG_CORE_FATAL("Failed to initialize database");
                        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
                        return EXIT_FAILURE;
                    }
                }
            }
        }
    }

    if (!db_migrated) {
        db_migrated = mud_db_migrate(); 
        if (!db_migrated) {
            LOG_CORE_ERROR("Failed to migrate database");
            shutdown_subsystems(db_loaded, config_loaded, log_loaded);
            return EXIT_FAILURE;
        } else {
            LOG_CORE_INFO("Database migrated successfully");
        }
    }

    mud_connection_table_init();

    if (net_loop_init) {
        LOG_CORE_INFO("Net loop already initialized!");
    } else {
        LOG_CORE_INFO("Net loop wasn't initialized.  Initializing...");
        net_loop_init = mud_net_loop_init();
        if (!net_loop_init) {
            LOG_CORE_ERROR("Failed to initialize net loop");
            shutdown_subsystems(db_loaded, config_loaded, log_loaded);
            return EXIT_FAILURE;
        } else {
            LOG_CORE_INFO("Mud Net Loop initialized successfully");
        }
    }

    if (listener_started) {
        LOG_CORE_INFO("Listenera already started");
    } else {
        LOG_CORE_INFO("Listener wasn't started.  Starting...");
        listener_started = mud_listener_start(port, max_connections);
        if (!listener_started) {
            LOG_CORE_ERROR("Failed to start listener");
            shutdown_subsystems(db_loaded, config_loaded, log_loaded);
            return EXIT_FAILURE;
        } else {
            LOG_CORE_INFO("Mud Listener started successfully");
        }
    }

    LOG_CORE_INFO("MUD server starting on port %d", port);
    if (mud_net_loop_run()) {
        LOG_CORE_INFO("MUD Server Net Loop Started Successfully on port %d", port);
    } else {
        LOG_CORE_ERROR("Failed to start MUD server Net Loop");
        shutdown_subsystems(db_loaded, config_loaded, log_loaded);
        return EXIT_FAILURE;
    }
    LOG_CORE_INFO("Startup checks completed successfully");

    // Cleanup
    LOG_CORE_INFO("Shutting down MUD server");
    shutdown_subsystems(db_loaded, config_loaded, log_loaded);
    return EXIT_SUCCESS;
}
