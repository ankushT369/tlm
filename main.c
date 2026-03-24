#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "db.h"
#include "linenoise.h"
#include "ops.h"
#include "security.h"
#include "tlm.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ITEMS 1000000
#define MAX_LEN 1024
#define MAX_PASS_LEN 32

char *items[MAX_ITEMS];
int item_count = 0;

int filtered[MAX_ITEMS];
int filtered_count = 0;

char query[MAX_LEN];
int query_len = 0;

int selected = 0;

static double current_time_millis() {
  struct timespec now;
  timespec_get(&now, TIME_UTC);
  return (double)now.tv_sec * 1000.0 + (double)now.tv_nsec / 1000000.0;
}

static void free_items() {
  for (int i = 0; i < item_count; i++) {
    free(items[i]);
  }

  item_count = 0;
}

int is_subsequence(const char *pattern, const char *text) {
  int j = 0;
  for (int i = 0; text[i] && pattern[j]; i++) {
    if (text[i] == pattern[j])
      j++;
  }
  return pattern[j] == '\0';
}

void filter_items() {
  filtered_count = 0;

  for (int i = 0; i < item_count; i++) {
    if (is_subsequence(query, items[i])) {
      filtered[filtered_count++] = i;
    }
  }

  if (selected >= filtered_count)
    selected = filtered_count - 1;

  if (selected < 0)
    selected = 0;
}

void draw() {
  clear();

  for (int i = 0; i < filtered_count && i < LINES - 2; i++) {
    if (i == selected)
      attron(A_REVERSE);

    mvprintw(i, 0, "%s", items[filtered[i]]);

    if (i == selected)
      attroff(A_REVERSE);
  }

  mvprintw(LINES - 1, 0, "Search: %s", query);
  refresh();
}

void load_history_from_db() {
  if (getHistoryFromDb_init() != 0)
    return;
  if (prepareUnionStmt() != 0)
    return;

  while (1) {
    int r = getHistoryFromDb_next();
    if (r <= 0)
      break;

    items[item_count++] = strdup(query_buffer);
  }
}

void m() {
  load_history_from_db();

  initscr();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(1);

  filter_items();
  draw();

  while (1) {
    int c = getch();

    if (c == 27)
      break; // ESC

    if (c == KEY_UP) {
      if (selected > 0)
        selected--;
    } else if (c == KEY_DOWN) {
      if (selected < filtered_count - 1)
        selected++;
    } else if (c == KEY_BACKSPACE || c == 127) {
      if (query_len > 0) {
        query[--query_len] = '\0';
      }
      filter_items();
    } else if (c == '\n') {
      endwin();
      // printf("Selected: %s\n", items[filtered[selected]]);
      return;
    } else if (c >= 32 && c <= 126) {
      query[query_len++] = c;
      query[query_len] = '\0';
      filter_items();
    }

    draw();
  }

  endwin();
}

static char *return_completion_substring(const char *buf) {
  int len;
  char **history = linenoiseHistoryGet(&len);
  for (int i = len - 1; i >= 0; i--) {
    if (strncmp(history[i], buf, strlen(buf)) == 0) {
      return history[i] + strlen(buf);
    }
  }
  return NULL;
}

void completion(const char *buf, linenoiseCompletions *lc) {
  if (buf[0] == 'c') {
    linenoiseAddCompletion(lc, "create");
  }

  if (buf[0] == 'a') {
    linenoiseAddCompletion(lc, "add");
  }

  if (buf[0] == 'u') {
    linenoiseAddCompletion(lc, "update");
  }

  if (buf[0] == 'r') {
    linenoiseAddCompletion(lc, "rm");
  }

  if (buf[0] == 's') {
    linenoiseAddCompletion(lc, "show");
  }
}

char *hints(const char *buf, int *color, int *bold) {
  if (!strcasecmp(buf, ""))
    return NULL;

  *color = 35;
  *bold = 0;
  return return_completion_substring(buf);
}

static void print_usage(const char *prog) {
  printf("Usage: %s [OPTIONS]\n", prog);
  printf("\n");
  printf("Options:\n");
  printf("  --help               Show this help message\n");
  printf("  --file <filename>    Use a specific database file\n");
}

int main(int argc, char **argv) {
  char *line;
  char *prgname = argv[0];
  int async = 1;
  int no_history = 1;
  double start_time, end_time, elapsed;

  const char *db_file = "tlm.db";

  for (int i = 1; i < argc; i++) {

    if (strcmp(argv[i], "--help") == 0) {
      print_usage(prgname);
      return 0;
    }

    else if (strcmp(argv[i], "--file") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "--file requires a filename\n");
        return 1;
      }

      db_file = argv[i + 1];
      i++;
    }

    else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      print_usage(prgname);
      return 1;
    }
  }

  /* Load database from file which includes history and internal data. */
  int ret = openFile(db_file);
  if (ret != 0)
    return 1;

  int exists = checkAuthExists();
  if (exists) {
    linenoiseMaskModeEnable();
    char *input = linenoise("Enter password: ");
    linenoiseMaskModeDisable();

    if (!input)
      return 1;

    if (!verifyPassword(input)) {
      printf("Wrong password!\n");
      linenoiseFree(input);
      return 1;
    }

    printf("Access granted.\n");
    linenoiseFree(input);
  }

  initHistoryDb();
  loadHistoryFromDb();

  /* Enable Multiline */
  linenoiseSetMultiLine(1);

  /* Set the completion callback. This will be called every time the
   * user uses the <tab> key. */
  linenoiseSetCompletionCallback(completion);
  linenoiseSetHintsCallback(hints);

  /* Now this is the main loop of the typical linenoise-based application.
   * The call to linenoise() will block as long as the user types something
   * and presses enter.
   *
   * The typed string is returned as a malloc() allocated string by
   * linenoise, so the user needs to free() it. */

  while (1) {
    if (async) {
      /* Asynchronous mode using the multiplexing API: wait for
       * data on stdin, and simulate async data coming from some source
       * using the select(2) timeout. */
      struct linenoiseState ls;
      char buf[1024];
      linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), "tlm> ");
      while (1) {
        fd_set readfds;
        int retval;

        FD_ZERO(&readfds);
        FD_SET(ls.ifd, &readfds);
        // tv.tv_sec = 1; // 1 sec timeout
        // tv.tv_usec = 0;

        retval = select(ls.ifd + 1, &readfds, NULL, NULL, NULL);
        if (retval == -1) {
          perror("select()");
          exit(1);
        } else if (retval) {
          line = linenoiseEditFeed(&ls);
          /* A NULL return means: line editing is continuing.
           * Otherwise the user hit enter or stopped editing
           * (CTRL+C/D). */
          if (line != linenoiseEditMore)
            break;
        } else {
          // Timeout occurred
        }
      }
      linenoiseEditStop(&ls);
      if (line == NULL)
        exit(0); /* Ctrl+D/C. */
    }

    /* Do something with the string. */
    if (line[0] != '\0' && line[0] != '/') {
      // printf("echo: '%s'\n", line); /* Now added for debugging purposes later
      // to be removed */
      char *query = processLine(line); /* Processing line starts here. */

      // printf("[INFO] Query: %s\n", query);
      if (query == NULL) {
        if (!no_history)
          addHistoryEntry(line);
        printf("Invalid Query\n");
        free(query);
        continue;
      }

      // printf("\n");

      /* Measure the db query latency */
      start_time = current_time_millis();
      ret = queryDb(currentCode, query);
      end_time = current_time_millis();

      printf("\n");

      elapsed = end_time - start_time;

      if (ret != 0) {
        printf("Execution Failed %.3fms\n", elapsed);
        free(query);
        continue;
      }

      printf("Execution Ok %.3fms\n", elapsed);
      if (!no_history)
        addHistoryEntry(line);

      free(query);

    } else if (!strncmp(line, "/auth", 5)) {
      int exists = checkAuthExists();
      char password[64];

      if (!exists) {
        memset(password, '\0', sizeof(password));

        linenoiseMaskModeEnable();

        char *input = linenoise("Enter password: ");
        if (input == NULL) {
          printf("Error reading password\n");
          return 1;
        }

        size_t len = strlen(input);

        if (len == 0 || len > 32) {
          printf("Password must be between 1 and 32 characters\n");
          linenoiseFree(input);
          linenoiseMaskModeDisable();
          return 1;
        }

        strncpy(password, input, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';

        linenoiseFree(input);
        linenoiseMaskModeDisable();

        if (authDB(password) != 0) {
          printf("Failed to initialize auth\n");
          return 1;
        }

        printf("Password set successfully.\n Next time you login you have to "
               "enter the password.\n Tip: You can verify your email for "
               "recovery\n");
      }
    } else if (!strncmp(line, "/noauth", 7)) {

    } else if (!strncmp(line, "/nohistory", 10)) {
      no_history = 1;
    } else if (!strncmp(line, "/historylen", 11)) {
      /* The "/historylen" command will change the history len. */
      // int len = atoi(line+11);
      // linenoiseHistorySetMaxLen(len);
    } else if (!strncmp(line, "/mask", 5)) {
      linenoiseMaskModeEnable();
    } else if (!strncmp(line, "/unmask", 7)) {
      linenoiseMaskModeDisable();
    } else if (!strncmp(line, "/search", 7)) {
      char *rest = line + 7;

      while (*rest == ' ')
        rest++;
      if (*rest != '\0') {
        // There IS more text after "/search"
        printf("Has argument: '%s'\n", rest);
      } else {
        if (!no_history)
          addHistoryEntry(line);
        m();
        printf("No argument!\n");
      }
      free_items();
    } else if (line[0] == '/') {
      printf("Unreconized command: %s\n", line);
    }

    free(line);
  }
  return 0;
}
