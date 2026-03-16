#ifndef __DB__
#define __DB__

#include "ops.h"
#include "sqlite3.h"

#define MAX_FILE_LEN 1024

typedef struct DbContext {
  sqlite3 *db;
  char file[1024];
  // sqlite3 *history;
} DbContext;

/* API */
int openFile(const char *file);
int queryDb(Ops code, char *sql);
int initHistoryDb();
void loadHistoryFromDb();
void addHistoryEntry(const char *line);

#endif // __DB__
