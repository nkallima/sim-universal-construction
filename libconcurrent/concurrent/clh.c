#include <clh.h>

void CLHLock(CLHLockStruct *l, int pid) {
    l->MyNode[pid]->locked = true;
    l->MyPred[pid] = (CLHLockNode *)SWAP(&l->Tail, (void *)l->MyNode[pid]);
    while (l->MyPred[pid]->locked == true) {
        resched();
    }
}

void CLHUnlock(CLHLockStruct *l, int pid) {
    l->MyNode[pid]->locked = false;
    l->MyNode[pid]= l->MyPred[pid];
#ifdef sparc
    StoreFence();
#endif
}

CLHLockStruct *CLHLockInit(uint32_t nthreads) {
    CLHLockStruct *l;
    int j;

    l = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockStruct));
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode));
    l->Tail->locked = false;

    l->MyNode = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode *));
    l->MyPred = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode *));
    for (j = 0; j < nthreads; j++) {
        l->MyNode[j] = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode));
        l->MyPred[j] = null;
    }

    return l;
}
