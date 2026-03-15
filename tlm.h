#ifndef __TLM__
#define __TLM__


struct tokens {
    char* token;
    unsigned int len;
};

/* API */
char* processLine(char* line);



#endif // __TLM__
