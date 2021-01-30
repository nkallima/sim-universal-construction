#include <mcs.h>

void MCSLock(MCSLockStruct *l, MCSThreadState *thread_state, int pid) {
    volatile MCSLockNode *prev;

    NonTSOFence();
    thread_state->MyNode->next = NULL;
    prev = SWAP(&l->Tail, (void *)thread_state->MyNode);

    if (prev != NULL) {
        l->Tail->locked = true;
        NonTSOFence();
        thread_state->MyNode->locked = true;
        prev->next = thread_state->MyNode;
        NonTSOFence();
    }
    while (thread_state->MyNode->locked == true) {
        resched();
    }
    FullFence();
}

void MCSUnlock(MCSLockStruct *l, MCSThreadState *thread_state, int pid) {
    NonTSOFence();
    if (thread_state->MyNode->next == NULL) {
        if (CASPTR(&l->Tail, thread_state->MyNode, NULL))
            return;

        while (thread_state->MyNode->next == NULL) {
            resched();
        }
    }
    thread_state->MyNode->next->locked = false;
    thread_state->MyNode->next = NULL;
    FullFence();
}

MCSLockStruct *MCSLockInit(void) {
    MCSLockStruct *l;

    l = getAlignedMemory(CACHE_LINE_SIZE, sizeof(MCSLockStruct));
    l->Tail = NULL;
    FullFence();

    return l;
}

void MCSThreadStateInit(MCSThreadState *st_thread, int pid) {
    st_thread->MyNode = getAlignedMemory(CACHE_LINE_SIZE, sizeof(MCSLockNode));
}
