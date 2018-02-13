#include <clh.h>

void CLHLock(CLHLockStruct *l, int pid) {
    l->MyNode[pid]->locked = true;
    l->MyPred[pid] = (CLHLockNode *)SWAP(&l->Tail, (void *)l->MyNode[pid]);
    while (l->MyPred[pid]->locked == true) {
#if N_THREADS > USE_CPUS
        resched();
#else
        ;
#endif
    }
}

void CLHUnlock(CLHLockStruct *l, int pid) {
    l->MyNode[pid]->locked = false;
    l->MyNode[pid]= l->MyPred[pid];
#ifdef sparc
    StoreFence();
#endif

#if N_THREADS > USE_CPUS
    resched();
#else
    ;
#endif
}

CLHLockStruct *CLHLockInit(void) {
    CLHLockStruct *l;
    int j;

    l = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockStruct));
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode));
    l->Tail->locked = false;

    for (j = 0; j < N_THREADS; j++) {
        l->MyNode[j] = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode));
        l->MyPred[j] = null;
    }

    return l;
}
