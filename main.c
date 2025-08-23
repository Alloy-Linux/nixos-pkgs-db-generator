#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sqlite3.h"
#include "cJSON.h"

#define INITIAL_BUF_SIZE 1048576

char* run_nix_env_json() {
    FILE *fp = popen("nix-env -qaP --json", "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run nix-env\n");
        return NULL;
    }

    size_t buf_size = INITIAL_BUF_SIZE;
    size_t total_read = 0;
    char *buffer = malloc(buf_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        pclose(fp);
        return NULL;
    }

    size_t bytes;
    while ((bytes = fread(buffer + total_read, 1, buf_size - total_read - 1, fp)) > 0) {
        total_read += bytes;
        if (buf_size - total_read <= 1) {
            buf_size *= 2;
            char *new_buffer = realloc(buffer, buf_size);
            if (!new_buffer) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(buffer);
                pclose(fp);
                return NULL;
            }
            buffer = new_buffer;
        }
    }
    buffer[total_read] = '\0';

    pclose(fp);
    return buffer;
}

void insert_packages(sqlite3 *db, cJSON *packages) {
    char *err_msg = NULL;

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA journal_mode = MEMORY;", NULL, NULL, NULL);

    const char *insert_sql = "INSERT OR REPLACE INTO packages (fullname, name, version, system, description) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    if (sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to begin transaction: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_finalize(stmt);
        return;
    }

    cJSON *pkg = NULL;
    cJSON_ArrayForEach(pkg, packages) {
        const char *fullname = pkg->string;
        if (!fullname) continue;

        cJSON *name_obj = cJSON_GetObjectItem(pkg, "name");
        cJSON *version_obj = cJSON_GetObjectItem(pkg, "version");
        cJSON *system_obj = cJSON_GetObjectItem(pkg, "system");
        cJSON *meta = cJSON_GetObjectItem(pkg, "meta");

        const char *name = name_obj ? name_obj->valuestring : "";
        const char *version = version_obj ? version_obj->valuestring : "";
        const char *system = system_obj ? system_obj->valuestring : "";
        const char *description = "";

        if (meta) {
            cJSON *desc_obj = cJSON_GetObjectItem(meta, "description");
            if (desc_obj) description = desc_obj->valuestring;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        sqlite3_bind_text(stmt, 1, fullname, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, version, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, system, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, description, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
        }
    }

    if (sqlite3_exec(db, "COMMIT;", NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to commit transaction: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    sqlite3_finalize(stmt);
}

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("Fetching from nix-env...\n");
    char *json_output = run_nix_env_json();
    if (!json_output) {
        return 1;
    }

    cJSON *packages = cJSON_Parse(json_output);
    if (!packages) {
        fprintf(stderr, "Failed to parse JSON\n");
        free(json_output);
        return 1;
    }

    int pkg_count = cJSON_GetArraySize(packages);
    printf("Loaded %d packages.\n", pkg_count);

    sqlite3 *db;
    if (sqlite3_open("nix_packages.db", &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        cJSON_Delete(packages);
        free(json_output);
        return 1;
    }

    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS packages ("
        "fullname TEXT PRIMARY KEY, "
        "name TEXT, "
        "version TEXT, "
        "system TEXT, "
        "description TEXT)";
    
    char *err_msg = NULL;
    if (sqlite3_exec(db, create_sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        cJSON_Delete(packages);
        free(json_output);
        return 1;
    }

    insert_packages(db, packages);

    sqlite3_close(db);
    cJSON_Delete(packages);
    free(json_output);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Success.\n");
    printf("Elapsed time: %.2f seconds\n", elapsed);

    return 0;
}
