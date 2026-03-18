#ifndef MUD_CONFIG_H
#define MUD_CONFIG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Load the server configuration from a file.
    Must bec alled before any mud_config_get_* functions
    @param path Path to mud.conf.  If NULL, defaults to "src/config/mud.conf"
    @return true on success, false if file is missing or has syntax errors
        On failure a human-readable error is printed to stderr
*/
bool mud_config_load(const char* path);

// Release all config memory
void_mud_config_shutdown(void);

/*  Scalar accessors
    The 'path' arg uses libconfig's dot-notation for nested lookup:
    exampmle -- "network.telnet.port" or "auth.max_failed_logins"

    Each returns the value if the key exists and has the right type, or 'default_val' otherwise
*/
const char* mud_config_get_string(const char* path, const char* default_val);
int         mud_config_get_int(const char* path, int default_val);
bool        mud_config_get_bool(const char* path, bool default_val);
double      mud_config_get_double(const char* path, double default_val);

#ifdef __cplusplus
}
#endif

#endif // MUD_CONFIG_H
