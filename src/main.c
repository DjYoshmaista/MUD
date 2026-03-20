#include "mud_config.h"
#include "mud_crypto.h"
#include "mud_db.h"
#include "mud_log.h"
#include "mud_utils.h"
#include "mud_json.h"
#include "mud_arena.h"
#include "mud_arena_string.h"
#include "mud_arena_temp.h"
#include "mud_buffer.h"
#include "mud_crypto.h"
#include "mud_hashmap.h"
#include "mud_str.h"
#include "mud_vector.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MUD_DB_PATH "data/mud.db"
#define WORLD_DB_PATH "data/world.db"
#define MUD_LOG_PATH "config/zlog.conf"
#define MUD_CONFIG_PATH "config/mud.conf"

#define TRY_INIT(func) \
    do { \
        if (!(func)) { \
            LOG_CORE_FATAL("failed to initialize %s", #func); \
            return false; \
        } \
    } while (0)

static bool config_loaded=false, db_open=false, crypto_init=false, log_cfg_ld=false;

MudArena* world_arena, textualia_arena;

bool run_initialization(void) {
    bool *db_open = mud_db_open(MUD_DB_PATH);
    if (TRY_INIT(mud_crypto_init()) || TRY_INIT(mud_config_init(MUD_CONFIG_PATH)) || TRY_INIT(mud_config_load(MUD_CONFIG_PATH)) || TRY_INIT(mud_log_init(MUD_LOG_PATH)) {
        config_loaded = true;
        db_open = true;
        log_cfg_ld = true;
        crypto_init = true;
        LOG_CORE_INFO("Initialization completed successfully");
        return true;
    } else {
        LOG_CORE_FATAL("Initialization failed");
        return false;
    }
}

static void shutdown_subsystems(bool config_loaded, bool db_open) {
    if (db_open) {
        mud_db_close();
    }
    if (config_loaded) {
        mud_config_shutdown();
    }
    mud_log_shutdown();
}

static char* world_init(void) {
    if (!run_initialization()) {
        shutdown_subsystems(config_loaded, db_open);
        return NULL;
    } else return "WORLD INITIALIZED";
}

int main(void) {
    mud_initialized = world_init();

    if (!mud_db_migrate()) {
        LOG_CORE_FATAL("Failed to migrate database schema");
        shutdown_subsystems(config_loaded, db_open);
        return EXIT_FAILURE;
    }

    LOG_CORE_INFO("Startup checks completed successfully");
    
    size_t world_arena_size = mud_config_get_int("world.arena_size", 8192);
    size_t textualia = mud_config_get_double("world.textualia.arena_size", 1024);

    mud_arena_create(world_arena);

    shutdown_subsystems(config_loaded, db_open);
    return EXIT_SUCCESS;
}
