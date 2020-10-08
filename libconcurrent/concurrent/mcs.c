#include <mcs.h>

void MCSLock(MCSLockStruct *l, MCSThreadState *thread_state, int pid) {
    volatile MCSLockNode *prev;
    
    thread_state->MyNode->next = NULL;
    prev = SWAP(&l->Tail, (void *)thread_state->MyNode);
    
    if (prev != NULL) {
        l->Tail->locked = true;
        thread_state->MyNode->locked = true;
        prev->next = thread_state->MyNode;
    }
    
    while (thread_state->MyNode->locked == true) {
        resched();
    }
}

void MCSUnlock(MCSLockStruct *l, MCSThreadState *thread_state, int pid) {
    if (thread_state->MyNode->next == NULL) {
        if (CASPTR(&l->Tail, thread_state->MyNode, NULL))
            return;

        while (thread_state->MyNode->next == NULL) {
            resched();
        }
    }
    thread_state->MyNode->next->locked = false;
    thread_state->MyNode->next = NULL;
}

MCSLockStruct *MCSLockInit(void) {
    MCSLockStruct *l;

    l = getAlignedMemory(CACHE_LINE_SIZE, sizeof(MCSLockStruct));
    l->Tail = NULL;

    return l;
}

void MCSThreadStateInit(MCSThreadState *st_thread, int pid) {
    st_thread->MyNode = getAlignedMemory(CACHE_LINE_SIZE, sizeof(MCSLockNode));
}
