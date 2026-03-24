#ifndef __DB__
#define __DB__

#include "ops.h"
#include "sqlite3.h"

#include <stddef.h>

#define BUF_LEN 1024

typedef struct DbContext {
  sqlite3 *db;
  char file[BUF_LEN];
} DbContext;

extern int header_printed;
extern char query_buffer[BUF_LEN];

/* API */
int openFile(const char *file);
int queryDb(Ops code, char *sql);
int initHistoryDb();
void loadHistoryFromDb();
void getHistoryFromDb();
void addHistoryEntry(const char *line);
int getHistoryFromDb_init();
int getHistoryFromDb_next(void);
int prepareUnionStmt();
DbContext getCtx();

#endif // __DB__
