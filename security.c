#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "db.h"
#include "security.h"
#include "sqlite3.h"

int authDB(const char *password) {
    char *err = NULL;
    int rc;

    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS tlm_auth ("
        "password_hash TEXT NOT NULL"
        ");";

    rc = sqlite3_exec(getCtx().db, create_sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", err);
        sqlite3_free(err);
        return 1;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)password, strlen(password), hash);

    char hash_hex[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash_hex + (i * 2), "%02x", hash[i]);
    }
    hash_hex[64] = '\0';

    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO tlm_auth (password_hash) VALUES ('%s');",
             hash_hex);

    rc = sqlite3_exec(getCtx().db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        printf("Insert error: %s\n", err);
        sqlite3_free(err);
        return 1;
    }

    return 0;
}

int checkAuthExists() {
    sqlite3_stmt *stmt;
    int rc;
    int exists = 0;

    const char *sql = "SELECT COUNT(*) FROM tlm_auth;";

    rc = sqlite3_prepare_v2(getCtx().db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Prepare failed: %s\n", sqlite3_errmsg(getCtx().db));
        return 0;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        if (count > 0) {
            exists = 1;
        }
    }

    sqlite3_finalize(stmt);
    return exists;
}

int verifyPassword(const char *password) {
    sqlite3_stmt *stmt;
    int rc;

    const char *sql = "SELECT password_hash FROM tlm_auth LIMIT 1;";

    rc = sqlite3_prepare_v2(getCtx().db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Prepare failed: %s\n", sqlite3_errmsg(getCtx().db));
        return 0;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)password, strlen(password), hash);

    char hash_hex[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash_hex + (i * 2), "%02x", hash[i]);
    }
    hash_hex[64] = '\0';

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 0;
    }

    const unsigned char *stored_hash = sqlite3_column_text(stmt, 0);

    int result = (strcmp((const char *)stored_hash, hash_hex) == 0);

    sqlite3_finalize(stmt);
    return result;
}
