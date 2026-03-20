PRAGMA journal_mode=DELETE;
PRAGMA foreign_keys=OFF;

DROP TABLE IF EXISTS account_notes;
DROP TABLE IF EXISTS array_documents;
DROP TABLE IF EXISTS json_documents;
DROP TABLE IF EXISTS mixed_values;
DROP TABLE IF EXISTS app_settings;
DROP TABLE IF EXISTS accounts;

CREATE TABLE accounts (
    id            INTEGER PRIMARY KEY,
    username      TEXT NOT NULL UNIQUE COLLATE NOCASE,
    password_hash TEXT NOT NULL,
    created_at    INTEGER NOT NULL,
    last_login_at INTEGER,
    failed_logins INTEGER NOT NULL DEFAULT 0,
    is_banned     INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE app_settings (
    cfg_key   TEXT PRIMARY KEY,
    cfg_value TEXT NOT NULL
);

CREATE TABLE mixed_values (
    item_key    TEXT PRIMARY KEY,
    text_value  TEXT NOT NULL,
    int_value   INTEGER NOT NULL,
    real_value  REAL NOT NULL,
    bool_value  INTEGER NOT NULL
);

CREATE TABLE json_documents (
    doc_key   TEXT PRIMARY KEY,
    json_text TEXT NOT NULL
);

CREATE TABLE array_documents (
    array_key  TEXT PRIMARY KEY,
    array_json TEXT NOT NULL
);

CREATE TABLE account_notes (
    note_id    INTEGER PRIMARY KEY,
    account_id INTEGER NOT NULL,
    note_key   TEXT NOT NULL,
    note_value TEXT NOT NULL,
    FOREIGN KEY (account_id) REFERENCES accounts(id)
);

INSERT INTO accounts (id, username, password_hash, created_at, last_login_at, failed_logins, is_banned) VALUES
    (1, 'fixture_alice', 'hash_alice_v1', 1700000000, 1700003600, 1, 0),
    (2, 'fixture_bob',   'hash_bob_v2',   1700100000, NULL,       3, 1),
    (3, 'fixture_cara',  'hash_cara_v3',  1700200000, 1700207200, 0, 0);

INSERT INTO app_settings (cfg_key, cfg_value) VALUES
    ('motd', 'Welcome to MUDv2'),
    ('region', 'NA-Central'),
    ('admin_email', 'admin@example.test');

INSERT INTO mixed_values (item_key, text_value, int_value, real_value, bool_value) VALUES
    ('alpha', 'hello world', 42, 3.14159, 1),
    ('beta',  'quest-ready', 7,  2.71828, 0);

INSERT INTO json_documents (doc_key, json_text) VALUES
    ('player_profile', '{"username":"fixture_alice","level":12,"title":"Builder","is_banned":false,"stats":{"str":14,"dex":11}}'),
    ('world_state',    '{"zone":"start","online":2,"weather":"rain","clock":{"hour":19,"minute":30}}');

INSERT INTO array_documents (array_key, array_json) VALUES
    ('starter_items', '["torch","rope","waterskin"]'),
    ('spawn_points',  '[101,102,103,104]');

INSERT INTO account_notes (note_id, account_id, note_key, note_value) VALUES
    (1, 1, 'rank', 'builder'),
    (2, 1, 'team', 'blue'),
    (3, 2, 'rank', 'moderator');

PRAGMA user_version=1;
