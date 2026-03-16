#include "db.h"
#include "linenoise.h"
#include "ops.h"

#include <stdio.h>
#include <string.h>

DbContext ctx;

int header_printed = 0;

/* callback function for SELECT queries */
static int callback(void *data, int argc, char **argv, char **colName) {
  if (!header_printed) {
    int len = 0;
    for (int i = 0; i < argc; i++) {
      len += strnlen(colName[i], 5);
      printf("%s ", colName[i]);
    }
    printf("\n");

    len += argc - 1;
    for (int i = 0; i < len; i++) {
      printf("%c", '-');
    }
    printf("\n");
    header_printed = 1;
  }

  for (int i = 0; i < argc; i++) {
    printf("%s ", argv[i] ? argv[i] : "NULL");
  }

  printf("\n");
  return 0;
}

static int history_callback(void *data, int argc, char **argv, char **cols) {
  if (argc > 0 && argv[0]) {
    linenoiseHistoryAdd(argv[0]);
  }
  return 0;
}

static void save_command_to_db(const char *cmd) {
  sqlite3_stmt *stmt;

  const char *sql = "INSERT OR IGNORE INTO tlm_history(command) VALUES(?);";

  if (sqlite3_prepare_v2(ctx.db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    fprintf(stderr, "Prepare failed\n");
    return;
  }

  sqlite3_bind_text(stmt, 1, cmd, -1, SQLITE_TRANSIENT);

  sqlite3_step(stmt);

  sqlite3_finalize(stmt);

  const char *limit_sql = "DELETE FROM tlm_history "
                          "WHERE id NOT IN ("
                          "SELECT id FROM tlm_history "
                          "ORDER BY id DESC LIMIT 10000);";

  char *err = NULL;

  if (sqlite3_exec(ctx.db, limit_sql, NULL, NULL, &err) != SQLITE_OK) {
    fprintf(stderr, "History limit error: %s\n", err);
    sqlite3_free(err);
  }
}

int openFile(const char *file) {
  int rc;
  if (strlen(file) >= MAX_FILE_LEN) {
    printf("Cannot create file\n");
    return 1;
  }

  strcpy(ctx.file, file);
  // copy const char* file to ctx.file whose type is char file[1024]

  char *err_msg = 0;
  /* open database */
  rc = sqlite3_open(ctx.file, &(ctx.db));

  if (rc != SQLITE_OK) {
    printf("Cannot open database\n");
    return 1;
  }

  return 0;
}

int queryDb(Ops code, char *sql) {
  char *err_msg = 0;
  int rc;

  if (code == SHOW) {
    rc = sqlite3_exec(ctx.db, sql, callback, 0, &err_msg);
  } else {
    rc = sqlite3_exec(ctx.db, sql, 0, 0, &err_msg);
  }

  header_printed = 0;

  if (rc != SQLITE_OK) {
    printf("SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return 1;
  }

  return 0;
}

int initHistoryDb() {
  const char *sql = "CREATE TABLE IF NOT EXISTS tlm_history ("
                    "id INTEGER PRIMARY KEY,"
                    "command TEXT NOT NULL"
                    ");";

  char *err = NULL;

  if (sqlite3_exec(ctx.db, sql, NULL, NULL, &err) != SQLITE_OK) {
    fprintf(stderr, "History table error: %s\n", err);
    sqlite3_free(err);
    return 1;
  }

  return 0;
}

void loadHistoryFromDb() {
  const char *sql = "SELECT command FROM tlm_history ORDER BY id ASC;";

  char *err = NULL;

  if (sqlite3_exec(ctx.db, sql, history_callback, NULL, &err) != SQLITE_OK) {
    fprintf(stderr, "Load history error: %s\n", err);
    sqlite3_free(err);
  }
}

void addHistoryEntry(const char *cmd) {
  if (!cmd || cmd[0] == '\0')
    return;

  linenoiseHistoryAdd(cmd);

  save_command_to_db(cmd);
}
