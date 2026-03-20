// src/mud_db.c
#include "mud_db.h"
#include "mud_log.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

static sqlite3* g_db = NULL;

bool mud_db_prepare(const char* sql, MudDbStmt** out_stmt) {
    if (g_db == NULL || sql == NULL || out_stmt == NULL) {
        LOG_DB_ERROR("Invalid arguments");
        return false;
    }

    *out_stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, (sqlite3_stmt**)out_stmt, NULL) != SQLITE_OK) {
        LOG_DB_ERROR("sqlite3_prepare_v2 failed: %s", sqlite3_errmsg(g_db));
        return false;
    }

    return true;
}

bool mud_db_bind_text(MudDbStmt* stmt, int index, const char* value) {
    if (stmt == NULL || index <= 0 || value == NULL) {
        return false;
    }

    return sqlite3_bind_text((sqlite3_stmt*)stmt, index, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool mud_db_bind_int64(MudDbStmt* stmt, int index, int64_t value) {
    if (stmt == NULL || index <= 0) {
        return false;
    }

    return sqlite3_bind_int64((sqlite3_stmt*)stmt, index, value) == SQLITE_OK;
}

MudDbStepResult mud_db_step(MudDbStmt* stmt) {
    int rc = 0;

    if (stmt == NULL) {
        return MUD_DB_STEP_ERROR;
    }

    rc = sqlite3_step((sqlite3_stmt*)stmt);
    if (rc == SQLITE_ROW) {
        return MUD_DB_STEP_ROW;
    }
    if (rc == SQLITE_DONE) {
        return MUD_DB_STEP_DONE;
    }

    return MUD_DB_STEP_ERROR;
}

const char* mud_db_column_text(MudDbStmt* stmt, int index) {
    const unsigned char* text = NULL;

    if (stmt == NULL || index < 0) {
        return NULL;
    }

    text = sqlite3_column_text((sqlite3_stmt*)stmt, index);
    return text ? (const char*)text : NULL;
}

int mud_db_column_int(MudDbStmt* stmt, int index, int default_val) {
    if (stmt == NULL || index < 0 || sqlite3_column_type((sqlite3_stmt*)stmt, index) == SQLITE_NULL) {
        return default_val;
    }

    return sqlite3_column_int((sqlite3_stmt*)stmt, index);
}

int64_t mud_db_column_int64(MudDbStmt* stmt, int index, int64_t default_val) {
    if (stmt == NULL || index < 0 || sqlite3_column_type((sqlite3_stmt*)stmt, index) == SQLITE_NULL) {
        return default_val;
    }

    return sqlite3_column_int64((sqlite3_stmt*)stmt, index);
}

double mud_db_column_double(MudDbStmt* stmt, int index, double default_val) {
    if (stmt == NULL || index < 0 || sqlite3_column_type((sqlite3_stmt*)stmt, index) == SQLITE_NULL) {
        return default_val;
    }

    return sqlite3_column_double((sqlite3_stmt*)stmt, index);
}

bool mud_db_column_bool(MudDbStmt* stmt, int index, bool default_val) {
    if (stmt == NULL || index < 0 || sqlite3_column_type((sqlite3_stmt*)stmt, index) == SQLITE_NULL) {
        return default_val;
    }

    return sqlite3_column_int((sqlite3_stmt*)stmt, index) != 0;
}

int mud_db_changes(void) {
    if (g_db == NULL) {
        return 0;
    }

    return sqlite3_changes(g_db);
}

void mud_db_finalize(MudDbStmt* stmt) {
    if (stmt != NULL) {
        sqlite3_finalize((sqlite3_stmt*)stmt);
    }
}

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

    // Enable WAL mode: allows readers and one writer to operate concurrently.
    // Essential for a MUD where the world-save thread and player sessions may try to write at the same time.
    if (!mud_db_exec("PRAGMA journal_mode=WAL;")) {
        LOG_DB_ERROR("PRAGMA journal_mode=WAL failed: %s", sqlite3_errmsg(g_db));
        mud_db_close();
        return false;
    }

    // Enforce foreign key constraints -- SQLite3 ignores them by default.
    if (!mud_db_exec("PRAGMA foreign_keys=ON;")) {
        LOG_DB_ERROR("PRAGMA foreign_keys=ON failed: %s", sqlite3_errmsg(g_db));
        mud_db_close();
        return false;
    }

    LOG_DB_INFO("Database opened: %s", path);
    return true;
}

void mud_db_close(void) {
    if (g_db != NULL) {
        LOG_DB_INFO("Closing database");
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

bool mud_db_is_open(void) {
    return g_db != NULL;
}

bool mud_db_exec(const char* sql) {
    if (g_db == NULL || sql == NULL) {
        LOG_DB_ERROR("Invalid arguments");
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
        LOG_DB_ERROR("Database not open");
        return false;
    }

    // Schema version tracking: store the current version in a user_version pragma.
    // When you add a new table or column, increment the version number and add a migration block below.

    int current_version = 0;
    MudDbStmt* stmt = NULL;
    LOG_DB_DEBUG("Checking current schema version");
    if (!mud_db_prepare("PRAGMA user_version;", &stmt)) {
        return false;
    }

    if (mud_db_step(stmt) == MUD_DB_STEP_ROW) {
        current_version = mud_db_column_int(stmt, 0, 0);
        LOG_DB_DEBUG("Current schema version: %d", current_version);
    } else {
        LOG_DB_ERROR("Failed to get current schema version: %s", sqlite3_errmsg(g_db));
        mud_db_finalize(stmt);
        return false;
    }

    mud_db_finalize(stmt);
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
    MudDbStmt* stmt = NULL;
    bool ok = false;

    if (g_db == NULL || username == NULL || password_hash == NULL) {
        return false;
    }

    if (!mud_db_prepare(SQL, &stmt)) {
        return false;
    }

    if (!mud_db_bind_text(stmt, 1, username) || !mud_db_bind_text(stmt, 2, password_hash)) {
        mud_db_finalize(stmt);
        return false;
    }

    ok = mud_db_step(stmt) == MUD_DB_STEP_DONE;
    mud_db_finalize(stmt);
    return ok;
}

bool mud_db_account_get_by_name(const char* username, MudDbAccount* out) {
    static const char* SQL =
        "SELECT id, username, password_hash, created_at, "
        "COALESCE(last_login_at, 0), failed_logins, is_banned "
        "FROM accounts WHERE username = ?;";
    MudDbStmt* stmt = NULL;
    bool ok = false;

    if (g_db == NULL || username == NULL || out == NULL) {
        return false;
    }

    if (!mud_db_prepare(SQL, &stmt)) {
        return false;
    }

    if (!mud_db_bind_text(stmt, 1, username)) {
        mud_db_finalize(stmt);
        return false;
    }

    if (mud_db_step(stmt) == MUD_DB_STEP_ROW) {
        memset(out, 0, sizeof(*out));
        out->id = mud_db_column_int64(stmt, 0, 0);
        snprintf(out->username, sizeof(out->username), "%s",
                 mud_db_column_text(stmt, 1) ? mud_db_column_text(stmt, 1) : "");
        snprintf(out->password_hash, sizeof(out->password_hash), "%s",
                 mud_db_column_text(stmt, 2) ? mud_db_column_text(stmt, 2) : "");
        out->created_at = mud_db_column_int64(stmt, 3, 0);
        out->last_login_at = mud_db_column_int64(stmt, 4, 0);
        out->failed_logins = mud_db_column_int(stmt, 5, 0);
        out->is_banned = mud_db_column_bool(stmt, 6, false);
        ok = true;
    }

    mud_db_finalize(stmt);
    return ok;
}

bool mud_db_account_update_login(int64_t account_id, int64_t timestamp) {
    MudDbStmt* stmt = NULL;
    bool ok = false;
    static const char* SQL =
        "UPDATE accounts "
        "SET last_login_at = ?, failed_logins = 0 "
        "WHERE id = ?;";

    if (g_db == NULL) {
        return false;
    }

    if (!mud_db_prepare(SQL, &stmt)) {
        return false;
    }

    if (!mud_db_bind_int64(stmt, 1, timestamp) || !mud_db_bind_int64(stmt, 2, account_id)) {
        mud_db_finalize(stmt);
        return false;
    }

    ok = mud_db_step(stmt) == MUD_DB_STEP_DONE && mud_db_changes() == 1;
    mud_db_finalize(stmt);
    return ok;
}

bool mud_db_account_increment_failed_logins(int64_t account_id) {
    MudDbStmt* stmt = NULL;
    bool ok = false;
    static const char* SQL =
        "UPDATE accounts SET failed_logins = failed_logins + 1 WHERE id = ?;";

    if (g_db == NULL) {
        return false;
    }

    if (!mud_db_prepare(SQL, &stmt)) {
        return false;
    }

    if (!mud_db_bind_int64(stmt, 1, account_id)) {
        mud_db_finalize(stmt);
        return false;
    }

    ok = mud_db_step(stmt) == MUD_DB_STEP_DONE && mud_db_changes() == 1;
    mud_db_finalize(stmt);
    return ok;
}

bool mud_db_account_reset_failed_logins(int64_t account_id) {
    MudDbStmt* stmt = NULL;
    bool ok = false;
    static const char* SQL =
        "UPDATE accounts SET failed_logins = 0 WHERE id = ?;";

    if (g_db == NULL) {
        return false;
    }

    if (!mud_db_prepare(SQL, &stmt)) {
        return false;
    }

    if (!mud_db_bind_int64(stmt, 1, account_id)) {
        mud_db_finalize(stmt);
        return false;
    }

    ok = mud_db_step(stmt) == MUD_DB_STEP_DONE && mud_db_changes() == 1;
    mud_db_finalize(stmt);
    return ok;
}
