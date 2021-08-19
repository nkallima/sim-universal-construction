#include <mcs.h>
#include <threadtools.h>

void MCSLock(MCSLockStruct *l, MCSThreadState *thread_state, int pid) {
    volatile MCSLockNode *prev;

    synchNonTSOFence();
    thread_state->MyNode->next = NULL;
    prev = synchSWAP(&l->Tail, (void *)thread_state->MyNode);

    if (prev != NULL) {
        l->Tail->locked = true;
        synchNonTSOFence();
        thread_state->MyNode->locked = true;
        synchNonTSOFence();
        prev->next = thread_state->MyNode;
        synchNonTSOFence();
    }
    while (thread_state->MyNode->locked == true) {
        synchResched();
    }
    synchFullFence();
}

void MCSUnlock(MCSLockStruct *l, MCSThreadState *thread_state, int pid) {
    synchNonTSOFence();
    if (thread_state->MyNode->next == NULL) {
        if (synchCASPTR(&l->Tail, thread_state->MyNode, NULL)) return;

        while (thread_state->MyNode->next == NULL) {
            synchResched();
        }
    }
    thread_state->MyNode->next->locked = false;
    thread_state->MyNode->next = NULL;
    synchFullFence();
}

MCSLockStruct *MCSLockInit(void) {
    MCSLockStruct *l;

    l = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(MCSLockStruct));
    l->Tail = NULL;
    synchFullFence();

    return l;
}

void MCSThreadStateInit(MCSThreadState *st_thread, int pid) {
    st_thread->MyNode = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(MCSLockNode));
}
