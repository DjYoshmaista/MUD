#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void remove_if_exists(const char* path) {
    if (path != NULL) {
        (void)remove(path);
    }
}

static void cleanup_sqlite_sidecars(const char* db_path) {
    static const char* suffixes[] = {"-journal", "-wal", "-shm"};
    size_t base_len;

    if (db_path == NULL) {
        return;
    }

    base_len = strlen(db_path);
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        size_t suffix_len = strlen(suffixes[i]);
        char* sidecar = (char*)malloc(base_len + suffix_len + 1U);
        if (sidecar == NULL) {
            continue;
        }

        memcpy(sidecar, db_path, base_len);
        memcpy(sidecar + base_len, suffixes[i], suffix_len + 1U);
        remove_if_exists(sidecar);
        free(sidecar);
    }
}

static char* read_file(const char* path, long* out_size) {
    FILE* fp = fopen(path, "rb");
    char* buf = NULL;
    long size = 0;

    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    buf = (char*)malloc((size_t)size + 1U);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }

    if (size > 0) {
        size_t nread = fread(buf, 1U, (size_t)size, fp);
        if (nread != (size_t)size) {
            free(buf);
            fclose(fp);
            return NULL;
        }
    }

    buf[size] = '\0';
    fclose(fp);

    if (out_size != NULL) {
        *out_size = size;
    }

    return buf;
}

int main(int argc, char** argv) {
    const char* sql_path;
    const char* db_path;
    char* sql = NULL;
    sqlite3* db = NULL;
    char* err_msg = NULL;
    long sql_size = 0;
    int rc;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <fixture.sql> <output.db>\n", argv[0]);
        return EXIT_FAILURE;
    }

    sql_path = argv[1];
    db_path = argv[2];

    sql = read_file(sql_path, &sql_size);
    if (sql == NULL) {
        fprintf(stderr, "failed to read sql fixture: %s\n", sql_path);
        return EXIT_FAILURE;
    }

    remove_if_exists(db_path);
    cleanup_sqlite_sidecars(db_path);

    rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "sqlite3_open(%s) failed: %s\n",
                db_path, db ? sqlite3_errmsg(db) : "no handle");
        free(sql);
        if (db != NULL) {
            sqlite3_close(db);
        }
        return EXIT_FAILURE;
    }

    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "sqlite3_exec fixture failed: %s\n",
                err_msg ? err_msg : "unknown error");
        sqlite3_free(err_msg);
        sqlite3_close(db);
        free(sql);
        return EXIT_FAILURE;
    }

    sqlite3_close(db);
    free(sql);

    printf("generated %s from %s (%ld bytes of SQL)\n", db_path, sql_path, sql_size);
    return EXIT_SUCCESS;
}
