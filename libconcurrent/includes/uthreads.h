#undef _FORTIFY_SOURCE

#ifndef _UTHREADS_H_
#    define _UTHREADS_H_

#    include <stdlib.h>
#    include <ucontext.h>
#    include <setjmp.h>

#    include <config.h>
#    include <primitives.h>

#    define FIBER_STACK 65536

typedef struct Fiber {
    ucontext_t context; /* Stores the current context */
    jmp_buf jmp;
    bool active;
} Fiber;

typedef struct FiberData {
    void *(*func)(void *);
    jmp_buf *cur;
    ucontext_t *prev;
    long arg;
} FiberData;

void initFibers(int max);
void fiberYield(void);
int spawnFiber(void *(*func)(void *), long arg);
void waitForAllFibers(void);

#endif
