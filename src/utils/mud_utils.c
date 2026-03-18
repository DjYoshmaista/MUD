#include "mud_log.h"

// Call when admin issues a "reload logging" command :: zlog_reload re-reads the config file without restarting the server
void mud_log_reload(const char* config_path) {
    if (zlog_reload(config_path) != 0) {
        LOG_ADMIN_ERROR("Failed to reload zlog config from '%s'", config_path);
    } else {
        LOG_ADMIN_INFO("zlog config reloaded from '%s'", config_path);
    }
}
