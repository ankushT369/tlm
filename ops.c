#include "ops.h"

#include <ctype.h>
#include <string.h>

Ops currentCode = INVALID;
const char *commands[5] = {"show", "create", "add", "rm", "update"};

/* Static Declaration */
static void string_to_lower(char *str);

static void string_to_lower(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    // Cast to unsigned char to avoid undefined behavior with negative char
    // values
    str[i] = (char)tolower((unsigned char)str[i]);
  }
}

Ops returnCommand(char *arg) {
  Ops ret = INVALID;
  if (!arg)
    return ret;

  string_to_lower(arg);

  if (!strcmp(arg, commands[SHOW])) {
    ret = SHOW;
    return ret;
  } else if (!strcmp(arg, commands[CREATE])) {
    ret = CREATE;
    return ret;
  } else if (!strcmp(arg, commands[ADD])) {
    ret = ADD;
    return ret;
  } else if (!strcmp(arg, commands[RM])) {
    ret = RM;
    return ret;
  } else if (!strcmp(arg, commands[UPDATE])) {
    ret = UPDATE;
    return ret;
  } else {
    ret = INVALID;
    return ret;
  }
}
