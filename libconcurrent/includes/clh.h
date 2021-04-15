#ifndef _CLH_H_
#define _CLH_H_

#include <primitives.h>
#include <stdbool.h>

typedef union CLHLockNode {
    bool locked;
    char align[CACHE_LINE_SIZE];
} CLHLockNode;

typedef struct CLHLockStruct {
    volatile CLHLockNode *Tail CACHE_ALIGN;
    volatile CLHLockNode **MyNode CACHE_ALIGN;
    volatile CLHLockNode **MyPred;
} CLHLockStruct;

void CLHLock(CLHLockStruct *l, int pid);
void CLHUnlock(CLHLockStruct *l, int pid);
CLHLockStruct *CLHLockInit(uint32_t nthreads);

#endif
