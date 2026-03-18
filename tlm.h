#ifndef __TLM__
#define __TLM__

#define MAX_LINK_LEN 4096

struct tokens {
  char *token;
  // char token[MAX_LINK_LEN];
  unsigned int len;
};

/* API */
char *processLine(char *line);

#endif // __TLM__
