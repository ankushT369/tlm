#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "tlm.h"

#define MAX_QUERY_LEN 8192

struct tokens token_array[10]; // Array of token structs
unsigned int token_count = 0;
char query_line[MAX_QUERY_LEN];

static void strip_quotes(char *str) {
  size_t len = strlen(str);

  if (len >= 2) {
    if ((str[0] == '\'' && str[len - 1] == '\'') ||
        (str[0] == '"' && str[len - 1] == '"')) {

      memmove(str, str + 1, len - 2);
      str[len - 2] = '\0';
    }
  }
}

static char *queryBuilderShowTables() {
  const char *sql = "SELECT name FROM sqlite_master WHERE type='table';";

  char *query = malloc(strlen(sql) + 1);
  if (!query)
    return NULL;

  strcpy(query, sql);

  return query;
}

static char *queryBuilderShowTable(char *table) {
  size_t size = strlen("SELECT * FROM ;") + strlen(table) + 1;

  char *query = malloc(size);
  if (!query)
    return NULL;

  sprintf(query, "SELECT * FROM %s;", table);

  return query;
}

static char *queryBuilderShow(struct tokens *arr, int count) {
  if (count != 2)
    return NULL;

  char *arg = arr[1].token;

  if (!strncmp(arg, "tlm_history", 11)) {
    return NULL;
  }

  if (!strncmp(arg, "tlm", 3)) {
    return queryBuilderShowTables();
  }

  return queryBuilderShowTable(arg);
}

static char *queryBuilderCreate(struct tokens *arr, int count) {
  if (count != 2)
    return NULL;

  char *table= arr[1].token;

  if (strcmp(table, "tlm") == 0) {
    printf("Error: 'tlm' is a reserved keyword.\n");
    return NULL;
  }

  if (strcmp(table, "tlm_history") == 0) {
    printf("Error: 'tlm_history' is a reserved keyword.\n");
    return NULL;
  }

  size_t size =
      strlen("CREATE TABLE  (id INTEGER PRIMARY KEY, link TEXT, title TEXT, UNIQUE(link, title));") +
      strlen(table) + 1;

  char *query = malloc(size);
  if (!query)
    return NULL;

  snprintf(query, size,
           "CREATE TABLE %s (id INTEGER PRIMARY KEY, link TEXT, title TEXT, UNIQUE(link, title));",
           table);

  return query;
}

static char *queryBuilderAdd(struct tokens *arr, int count) {
  if (count != 4)
    return NULL;

  char *table = arr[1].token;
  char *link = arr[2].token;
  char *title = arr[3].token;

  if (strcmp(table, "tlm") == 0) {
    printf("Error: 'tlm' is a reserved keyword.\n");
    return NULL;
  }

  if (strcmp(table, "tlm_history") == 0) {
    printf("Error: 'tlm_history' is a reserved keyword.\n");
    return NULL;
  }

  strip_quotes(title);

  size_t size = strlen("INSERT INTO  (link, title) VALUES ('', '');") +
                strlen(table) + strlen(link) + strlen(title) + 1;

  char *query = malloc(size);
  if (!query)
    return NULL;

  sprintf(query, "INSERT INTO %s (link, title) VALUES ('%s', '%s');", table,
          link, title);

  return query;
}

static char *queryBuilderRm(struct tokens *arr, int count) {
  if (count != 3)
    return NULL;

  char *table = arr[1].token;
  char *condition = arr[2].token;

  if (strcmp(table, "tlm") == 0) {
    printf("Error: 'tlm' is a reserved keyword.\n");
    return NULL;
  }

  if (strcmp(table, "tlm_history") == 0) {
    printf("Error: 'tlm_history' is a reserved keyword.\n");
    return NULL;
  }

  size_t size =
      strlen("DELETE FROM  WHERE ;") + strlen(table) + strlen(condition) + 1;

  char *query = malloc(size);
  if (!query)
    return NULL;

  sprintf(query, "DELETE FROM %s WHERE %s;", table, condition);

  return query;
}

static char *queryBuilderUpdate(struct tokens *arr, int count) {
  if (count < 3 || count > 5) {
    printf("Error: Not enough tokens for UPDATE query\n");
    return NULL;
  }

  char *table = arr[1].token;

  if (strcmp(table, "tlm") == 0) {
    printf("Error: 'tlm' is a reserved keyword.\n");
    return NULL;
  }

  if (strcmp(table, "tlm_history") == 0) {
    printf("Error: 'tlm_history' is a reserved keyword.\n");
    return NULL;
  }

  size_t query_size = strlen("UPDATE  SET ") + strlen(table) + 1;

  int set_clause_start = 2;

  for (int i = set_clause_start; i < count; i++) {
    query_size += strlen(arr[i].token) + 2;
  }

  char *query = malloc(query_size);
  if (!query) {
    printf("Error: Memory allocation failed\n");
    return NULL;
  }

  sprintf(query, "UPDATE %s SET ", table);

  for (int i = set_clause_start; i < count; i++) {
    strcat(query, arr[i].token);

    if (i < count - 1) {
      strcat(query, ", ");
    }
  }

  return query;
}

static char *queryBuilder(Ops code, struct tokens *arr, int count) {
  char *query;
  if (code == SHOW) {
    query = queryBuilderShow(arr, count);
    if (query) {
      return query;
    } else {
      return NULL;
    }
  } else if (code == CREATE) {
    query = queryBuilderCreate(arr, count);
    if (query) {
      return query;
    } else {
      return NULL;
    }
  } else if (code == ADD) {
    query = queryBuilderAdd(arr, count);
    if (query) {
      return query;
    } else {
      return NULL;
    }
  } else if (code == RM) {
    query = queryBuilderRm(arr, count);
    if (query) {
      return query;
    } else {
      return NULL;
    }
  } else if (code == UPDATE) {
    query = queryBuilderUpdate(arr, count);
    if (query) {
      return query;
    } else {
      return NULL;
    }
  } else {
    return query;
  }

  return NULL;
}

void tokenize(char *str) {
  unsigned int len = strlen(str);
  unsigned int offset = 0;

  char *token;

  int index[len];
  memset(index, 0, sizeof(index));

  bool single_quote = false;
  bool double_quote = false;

  while (offset < len) {
    if (str[offset] == '\'' && !double_quote) {
      single_quote = !single_quote;
      offset++;
      continue;
    }

    if (str[offset] == '"' && !single_quote) {
      double_quote = !double_quote;
      offset++;
      continue;
    }

    if (single_quote || double_quote) {
      if (str[offset] == ' ') {
        str[offset] = '_';
        index[offset] = 1;
      }
    }

    offset++;
  }

  // Get the first token
  token = strtok(str, " ");

  // Walk through remaining tokens
  while (token != NULL && token_count < 10) {
    // Allocate memory and copy the token
    token_array[token_count].token = malloc(strlen(token) + 1);
    strcpy(token_array[token_count].token, token);
    token_array[token_count].len = strlen(token);

    token_count++;
    token = strtok(NULL, " ");
  }

  unsigned int str_pos = 0;

  for (unsigned int i = 0; i < token_count; i++) {
    for (unsigned int j = 0; j < token_array[i].len; j++) {
      // Check if this position in original string had a space
      if (str_pos < len - (token_count - 1)) {
        // printf("str_pos : %d\n", str_pos);
        if (index[str_pos] == 1) {
          // printf("tokendddd: %c\n", token_array[i].token[j]);
          token_array[i].token[j] = ' ';
        }
        str_pos++;
      }
    }
    // str_pos += token_array[i].len + 1;  // Move to next token position (+1
    // for space)
    str_pos++;
  }
}

static void resetTokens(void) {
  for (int i = 0; i < 10; i++) {
    if (token_array[i].token != NULL) {
      free(token_array[i].token);
      token_array[i].token = NULL;
    }

    token_array[i].len = 0;
  }

  token_count = 0;
  memset(token_array, 0, sizeof(token_array));
}

static void debug_print() {
  printf("Tokens and their lengths:\n");
  for (unsigned int i = 0; i < token_count; i++) {
    printf("Token %d: \"%s\" (length: %u)\n", i, token_array[i].token,
           token_array[i].len);
  }
}

char *processLine(char *line) {
  // Copy the original line to the buffer
  memcpy(query_line, line, strnlen(line, MAX_QUERY_LEN));
  tokenize(query_line);

  // printf("tokename: %s\n", token_array[0].token);
  Ops code = returnCommand(token_array[0].token);
  if (code == INVALID) {
    currentCode = INVALID;
    resetTokens();
    return NULL;
  }

  debug_print();
  char *query = queryBuilder(code, token_array, token_count);
  currentCode = code;

  if (!query) {
    resetTokens();
    return NULL;
  }

  memset(query_line, '\0', MAX_QUERY_LEN);
  resetTokens();
  return query;
}
