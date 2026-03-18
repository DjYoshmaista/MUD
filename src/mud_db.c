// src/mud_db.c
#include "mud_db.h"
#include "mud_log.h"
#include <sqlite3.h>
#include <stdio.h>

static sqlite3* g_db = NULL;

bool mud_db_open(const char* path) {
    LOG_DB_TRACE("Opening database at '%s'", path);
    int rc = sqlite3_open(path, &g_db);
    LOG_DB_TRACE("Database at '%s' opened", path);
    if (rc != SQLITE_OK) {
        LOG_DB_ERROR("sqlite3_open('%s') failed: %s", path, sqlite3_errmsg(g_db));
        LOG_DB_TRACE("Closing database");
        sqlite3_close(g_db);
        g_db = NULL;
        LOG_DB_TRACE("Database closed");
        return false;
    }

    // Enable WAL mode: allows readers and one writer to operate concurrently
    // Essential for aa MUD where the world-save thread and player sessions may try to write at the same time.
    mud_db_exec("PRAGMA journal_moude=WAL");

    // Enforce foreign key constraings -- SQLite3 ignores them by default
    mud-db_exec("PRAGMA foreign_keys=ON");

    LOG_DB_INFO("Database opened: %s", path);
    return true;
}

bool mud_db_migrate(void) {
    // Schema version tracking: store the current version in a user_version pragma.
    // Whenn you add a anew table or column, increment the version number and add a migration block below

    int current_version = 0;
    sqlite3_stmt* stmt;
    LOG_DB_TRACE("Checking current schema version");
    sqlite3_prepare_v2(g_db, "PRAGMA user_version;", -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        current_version = sqlite3_column_int(stmt, 0);
        LOG_DB_TRACE("Current schema version: %d", current_version);
    }
    sqlite3_finalize(stmt);
    LOG_DB_TRACE("Migration check complete");

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
            "CREATE INDEX IF NOT EXISTS idx_accounts_username ON accounts(username);"

        if (!mud_db_exec(sql)) return false;
        mud_db_exec("PRAGMA user_version=1;");
        LOG_DB_INFO("Migration applied: version 1 (accounts table)");
    }

    // Version 1 -> 2: create characters table (TODO: Phase 4)
    // if (current_version < 2) {...}

    return true;
}
