#include <ccsynch.h>
#include <stdbool.h>
#include <primitives.h>
#include <threadtools.h>

static const int CCSYNCH_HELP_FACTOR = 10;

RetVal CCSynchApplyOp(CCSynchStruct *l, CCSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile CCSynchNode *p;
    volatile CCSynchNode *cur;
    CCSynchNode *next_node, *tmp_next;
    int help_bound = CCSYNCH_HELP_FACTOR * l->nthreads;
    int counter = 0;

    next_node = st_thread->next;
    next_node->next = NULL;
    next_node->locked = true;
    synchNonTSOFence();
    next_node->completed = false;

    cur = (CCSynchNode *)synchSWAP(&l->Tail, next_node);
    cur->arg_ret = arg;
    cur->pid = pid;
    synchNonTSOFence();
    cur->next = (CCSynchNode *)next_node;
    st_thread->next = (CCSynchNode *)cur;
    synchNonTSOFence();

    while (cur->locked) { // spinning
        synchResched();
    }
    if (cur->completed) // I have been helped
        return cur->arg_ret;
#ifdef DEBUG
    l->rounds++;
#endif
    p = cur; // I am not been helped
    while (p->next != NULL && counter < help_bound) {
        synchStorePrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        tmp_next = p->next;
        p->arg_ret = sfunc(state, p->arg_ret, p->pid);
        synchNonTSOFence();
        p->completed = true;
        synchNonTSOFence();
        p->locked = false;
        p = tmp_next;
    }
    synchNonTSOFence();
    p->locked = false; // Unlock the next one
    synchStoreFence();

    return cur->arg_ret;
}

void CCSynchStructInit(CCSynchStruct *l, uint32_t nthreads) {
    l->nthreads = nthreads;

    if (synchGetMachineModel() == INTEL_X86_MACHINE) {
        l->nodes = NULL;
        l->Tail = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(CCSynchNode));
    } else {
        l->nodes = synchGetAlignedMemory(CACHE_LINE_SIZE, (nthreads + 1) * sizeof(CCSynchNode));
        l->Tail = &l->nodes[nthreads];
    }

#ifdef DEBUG
    l->rounds = l->counter = 0;
#endif

    l->Tail->next = NULL;
    l->Tail->locked = false;
    l->Tail->completed = false;

    synchStoreFence();
}

void CCSynchThreadStateInit(CCSynchStruct *l, CCSynchThreadState *st_thread, int pid) {
    if (synchGetMachineModel() == INTEL_X86_MACHINE) {
        st_thread->next = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(CCSynchStruct));
    } else {
        st_thread->next = &l->nodes[pid];
    }
}
