#include "db.h"
#include "linenoise.h"
#include "ops.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

DbContext ctx;

#define MAX_LEN 8192

int header_printed = 0;

sqlite3_stmt *g_frag_stmt = NULL;  // for fragments
sqlite3_stmt *g_union_stmt = NULL; // for final union query
char query_buffer[BUF_LEN];

/* callback function for SELECT queries */
static int callback(void *data __attribute__((unused)), int argc, char **argv,
                    char **colName __attribute__((unused))) {

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

static int build_final_union_sql(char *out, size_t outsz) {
  out[0] = '\0';

  while (sqlite3_step(g_frag_stmt) == SQLITE_ROW) {
    const unsigned char *txt = sqlite3_column_text(g_frag_stmt, 0);
    if (!txt)
      continue;

    strncat(out, (const char *)txt, outsz - strlen(out) - 1);
  }

  // Remove trailing "UNION ALL " if present
  size_t len = strlen(out);
  const char *ending = "UNION ALL ";
  size_t endlen = strlen(ending);

  if (len >= endlen && memcmp(out + len - endlen, ending, endlen) == 0) {
    out[len - endlen] = '\0';
  }

  sqlite3_finalize(g_frag_stmt);
  g_frag_stmt = NULL;

  return 0;
}

static int history_callback(void *data __attribute__((unused)), int argc,
                            char **argv, char **cols __attribute__((unused))) {
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
  umask(077);

  if (strlen(file) >= BUF_LEN) {
    printf("Cannot create file\n");
    return 1;
  }

  strcpy(ctx.file, file);
  // copy const char* file to ctx.file whose type is char file[1024]

  /* open database */
  rc = sqlite3_open(ctx.file, &(ctx.db));

  if (rc != SQLITE_OK) {
    printf("Cannot open database: %s\n", sqlite3_errmsg(ctx.db));
    return 1;
  }

  chmod(ctx.file, 0600);

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

int getHistoryFromDb_init() {
  const char *sql = "SELECT 'SELECT * FROM ' || name || ' UNION ALL ' "
                    "FROM sqlite_master "
                    "WHERE type='table' "
                    "AND name <> 'tlm_history' "
                    "AND name NOT LIKE 'sqlite_%';";

  if (g_frag_stmt) {
    sqlite3_finalize(g_frag_stmt);
    g_frag_stmt = NULL;
  }

  int rc = sqlite3_prepare_v2(ctx.db, sql, -1, &g_frag_stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "prepare fragments error: %s\n", sqlite3_errmsg(ctx.db));
    return -1;
  }

  return 0;
}

int prepareUnionStmt() {
  char final_sql[MAX_LEN];
  build_final_union_sql(final_sql, sizeof(final_sql));

  if (strlen(final_sql) == 0) {
    fprintf(stderr, "No tables found.\n");
    return -1;
  }

  // Prepare real UNION query
  int rc = sqlite3_prepare_v2(ctx.db, final_sql, -1, &g_union_stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "prepare union error: %s\n", sqlite3_errmsg(ctx.db));
    return -1;
  }

  return 0;
}

int getHistoryFromDb_next(void) {
  if (!g_union_stmt)
    return -1;

  int rc = sqlite3_step(g_union_stmt);

  if (rc == SQLITE_ROW) {
    query_buffer[0] = '\0';

    int cols = sqlite3_column_count(g_union_stmt);
    for (int i = 0; i < cols; i++) {
      const char *txt = (const char *)sqlite3_column_text(g_union_stmt, i);
      strcat(query_buffer, txt ? txt : "NULL");
      if (i < cols - 1)
        strcat(query_buffer, " ");
    }

    return 1; // delivered real row
  }

  if (rc == SQLITE_DONE) {
    sqlite3_finalize(g_union_stmt);
    g_union_stmt = NULL;
    return 0;
  }

  fprintf(stderr, "step error\n");
  return -1;
}

void addHistoryEntry(const char *cmd) {
  if (!cmd || cmd[0] == '\0')
    return;

  linenoiseHistoryAdd(cmd);

  save_command_to_db(cmd);
}

DbContext getCtx() { return ctx; }
