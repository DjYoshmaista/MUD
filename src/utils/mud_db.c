// src/mud_db.c
#include "mud_db.h"
#include "mud_log.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

static sqlite3* g_db = NULL;

bool mud_db_open(const char* path) {
    if (g_db != NULL || path == NULL || path[0] == '\0') {
        return false;
    }

    LOG_DB_DEBUG("Opening database at '%s'", path);
    int rc = sqlite3_open(path, &g_db);
    if (rc != SQLITE_OK) {
        LOG_DB_ERROR("sqlite3_open('%s') failed: %s", path, sqlite3_errmsg(g_db));
        sqlite3_close(g_db);
        g_db = NULL;
        return false;
    }

    // Enable WAL mode: allows readers and one writer to operate concurrently
    // Essential for aa MUD where the world-save thread and player sessions may try to write at the same time.
    if (!mud_db_exec("PRAGMA journal_mode=WAL;")) {
        mud_db_close();
        return false;
    }

    // Enforce foreign key constraings -- SQLite3 ignores them by default
    if (!mud_db_exec("PRAGMA foreign_keys=ON;")) {
        mud_db_close();
        return false;
    }

    LOG_DB_INFO("Database opened: %s", path);
    return true;
}

void mud_db_close(void) {
    if (g_db != NULL) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

bool mud_db_is_open(void) {
    return g_db != NULL;
}

bool mud_db_exec(const char* sql) {
    if (g_db == NULL || sql == NULL) {
        return false;
    }

    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_DB_ERROR("sqlite3_exec failed: %s", err_msg ? err_msg : "unknown error");
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

bool mud_db_migrate(void) {
    if (g_db == NULL) {
        return false;
    }

    // Schema version tracking: store the current version in a user_version pragma.
    // Whenn you add a anew table or column, increment the version number and add a migration block below

    int current_version = 0;
    sqlite3_stmt* stmt = NULL;
    LOG_DB_DEBUG("Checking current schema version");
    if (sqlite3_prepare_v2(g_db, "PRAGMA user_version;", -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        current_version = sqlite3_column_int(stmt, 0);
        LOG_DB_DEBUG("Current schema version: %d", current_version);
    } else {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    LOG_DB_DEBUG("Migration check complete");

    // Version 0 -> 1: Create initial tables
    if (current_version < 1) {
        const char* sql =
            "CREATE TABLE IF NOT EXISTS accounts ("
            "   id              INTEGER PRIMARY KEY AUTOINCREMENT,"
            "   username        TEXT    NOT NULL UNIQUE COLLATE NOCASE,"
            "   password_hash   TEXT    NOT NULL,"
            "   created_at      INTEGER NOT NULL DEFAULT (unixepoch()),"
            "   last_login_at   INTEGER,"
            "   failed_logins   INTEGER NOT NULL DEFAULT 0,"
            "   is_banned       INTEGER NOT NULL DEFAULT 0"
            ");"
            "CREATE INDEX IF NOT EXISTS idx_accounts_username ON accounts(username);";

        if (!mud_db_exec(sql)) return false;
        mud_db_exec("PRAGMA user_version=1;");
        LOG_DB_INFO("Migration applied: version 1 (accounts table)");
    }

    // Version 1 -> 2: create characters table (TODO: Phase 4)
    // if (current_version < 2) {...}

    return true;
}

bool mud_db_account_insert(const char* username, const char* password_hash) {
    static const char* SQL =
        "INSERT INTO accounts (username, password_hash) VALUES (?, ?);";
    sqlite3_stmt* stmt = NULL;
    bool ok = false;

    if (g_db == NULL || username == NULL || password_hash == NULL) {
        return false;
    }

    if (sqlite3_prepare_v2(g_db, SQL, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_TRANSIENT);
    ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool mud_db_account_get_by_name(const char* username, MudDbAccount* out) {
    static const char* SQL =
        "SELECT id, username, password_hash, created_at, "
        "COALESCE(last_login_at, 0), failed_logins, is_banned "
        "FROM accounts WHERE username = ?;";
    sqlite3_stmt* stmt = NULL;
    bool ok = false;

    if (g_db == NULL || username == NULL || out == NULL) {
        return false;
    }

    if (sqlite3_prepare_v2(g_db, SQL, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        memset(out, 0, sizeof(*out));
        out->id = sqlite3_column_int64(stmt, 0);
        snprintf(out->username, sizeof(out->username), "%s",
                 (const char*)sqlite3_column_text(stmt, 1));
        snprintf(out->password_hash, sizeof(out->password_hash), "%s",
                 (const char*)sqlite3_column_text(stmt, 2));
        out->created_at = sqlite3_column_int64(stmt, 3);
        out->last_login_at = sqlite3_column_int64(stmt, 4);
        out->failed_logins = sqlite3_column_int(stmt, 5);
        out->is_banned = sqlite3_column_int(stmt, 6) != 0;
        ok = true;
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool mud_db_account_update_login(int64_t account_id, int64_t timestamp) {
    sqlite3_stmt* stmt = NULL;
    bool ok = false;
    static const char* SQL =
        "UPDATE accounts "
        "SET last_login_at = ?, failed_logins = 0 "
        "WHERE id = ?;";

    if (g_db == NULL) {
        return false;
    }

    if (sqlite3_prepare_v2(g_db, SQL, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, timestamp);
    sqlite3_bind_int64(stmt, 2, account_id);
    ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(g_db) == 1;
    sqlite3_finalize(stmt);
    return ok;
}

bool mud_db_account_increment_failed_logins(int64_t account_id) {
    sqlite3_stmt* stmt = NULL;
    bool ok = false;
    static const char* SQL =
        "UPDATE accounts SET failed_logins = failed_logins + 1 WHERE id = ?;";

    if (g_db == NULL) {
        return false;
    }

    if (sqlite3_prepare_v2(g_db, SQL, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, account_id);
    ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(g_db) == 1;
    sqlite3_finalize(stmt);
    return ok;
}

bool mud_db_account_reset_failed_logins(int64_t account_id) {
    sqlite3_stmt* stmt = NULL;
    bool ok = false;
    static const char* SQL =
        "UPDATE accounts SET failed_logins = 0 WHERE id = ?;";

    if (g_db == NULL) {
        return false;
    }

    if (sqlite3_prepare_v2(g_db, SQL, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, account_id);
    ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(g_db) == 1;
    sqlite3_finalize(stmt);
    return ok;
}
