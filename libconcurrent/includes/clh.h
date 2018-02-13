#ifndef _CLH_H_
#define _CLH_H_

#include <stdlib.h>
#include <pthread.h>

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif

#include <primitives.h>
#include <config.h>

typedef union CLHLockNode {
    bool locked;
    char align[CACHE_LINE_SIZE];
} CLHLockNode;

typedef struct CLHLockStruct {
    volatile CLHLockNode *Tail CACHE_ALIGN;
    volatile CLHLockNode *MyNode[N_THREADS] CACHE_ALIGN;
    volatile CLHLockNode *MyPred[N_THREADS] CACHE_ALIGN;
} CLHLockStruct;


void CLHLock(CLHLockStruct *l, int pid);
void CLHUnlock(CLHLockStruct *l, int pid);
CLHLockStruct *CLHLockInit(void);

#endif
