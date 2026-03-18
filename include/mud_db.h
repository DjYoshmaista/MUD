#ifndef MUD_DB_H
#define MUD_DB_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Database Lifecycle
bool    mud_db_open(const char* path);
void    mud_db_close(void);
bool    mud_db_is_open(void);

// Schema Management
bool    mud_db_migrate(void);       // Creates/upgrades tables to current schema version

// Raw execution (for DDL and one-off statements that don't return rows)
bool    mud_db_exec(const char* sql);

// Account operations (examples - more in later phases)
typedef struct {
    int64_t         id;
    char            username[64];
    char            password_hash[128];
    int64_t         created_at;
    int64_t         last_login_at;
    int             failed_logins;
    bool            is_banned;
} MudDbAccount;

bool    mud_db_account_insert(const char* username, const char* password_hash);
bool    mud_db_account_get_by_name(const char* username, MudDbAccount* out);
bool    mud_db_account_update_login(int64_t account_id, int64_t timestamp);
bool    mud_db_account_increment_failed_logins(int64_t account_id);
bool    mud_db_account_reset_failed_logins(int64_t account_id);

#ifdef __cplusplus
}
#endif

#endif // MUD_DB_H
