#include <clh.h>
#include <primitives.h>
#include <threadtools.h>

void CLHLock(CLHLockStruct *l, int pid) {
    synchNonTSOFence();
    l->MyNode[pid]->locked = true;
    l->MyPred[pid] = (CLHLockNode *)synchSWAP(&l->Tail, (void *)l->MyNode[pid]);
    while (l->MyPred[pid]->locked == true) {
        synchResched();
    }
    synchFullFence();
}

void CLHUnlock(CLHLockStruct *l, int pid) {
    synchNonTSOFence();
    l->MyNode[pid]->locked = false;
    l->MyNode[pid] = l->MyPred[pid];
    synchFullFence();
}

CLHLockStruct *CLHLockInit(uint32_t nthreads) {
    CLHLockStruct *l;
    int j;

    l = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockStruct));
    l->Tail = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode));
    l->Tail->locked = false;

    l->MyNode = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(CLHLockNode *));
    l->MyPred = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(CLHLockNode *));
    for (j = 0; j < nthreads; j++) {
        l->MyNode[j] = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHLockNode));
        l->MyPred[j] = NULL;
    }
    synchFullFence();

    return l;
}
