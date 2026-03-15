#ifndef __OPS__
#define __OPS__

typedef enum Ops {
    SHOW,
    CREATE,
    ADD,
    RM,
    UPDATE,
    INVALID,
} Ops;

extern Ops currentCode;

/* API */
Ops returnCommand(char* arg);


#endif  // __OPS__
