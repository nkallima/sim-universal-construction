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
    next_node->completed = false;

    cur = (CCSynchNode *)SWAP(&l->Tail, next_node);
    cur->arg_ret = arg;
    cur->pid = pid;
    cur->next = (CCSynchNode *)next_node;
    st_thread->next = (CCSynchNode *)cur;

    while (cur->locked) { // spinning
        resched();
    }
    if (cur->completed) // I have been helped
        return cur->arg_ret;
#ifdef DEBUG
    l->rounds++;
#endif
    p = cur; // I am not been helped
    while (p->next != NULL && counter < help_bound) {
        StorePrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        tmp_next = p->next;
        p->arg_ret = sfunc(state, p->arg_ret, p->pid);
        NonTSOFence();
        p->completed = true;
        NonTSOFence();
        p->locked = false;
        p = tmp_next;
    }
    NonTSOFence();
    p->locked = false; // Unlock the next one
    StoreFence();

    return cur->arg_ret;
}

void CCSynchStructInit(CCSynchStruct *l, uint32_t nthreads) {
    l->nthreads = nthreads;

#ifdef SYNCH_COMPACT_ALLOCATION
    l->nodes = getAlignedMemory(CACHE_LINE_SIZE, (nthreads + 1) * sizeof(CCSynchNode));
    l->Tail = &l->nodes[nthreads];
#else
    l->nodes = NULL;
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CCSynchNode));
#endif

#ifdef DEBUG
    l->rounds = l->counter = 0;
#endif

    l->Tail->next = NULL;
    l->Tail->locked = false;
    l->Tail->completed = false;

    StoreFence();
}

void CCSynchThreadStateInit(CCSynchStruct *l, CCSynchThreadState *st_thread, int pid) {
#ifdef SYNCH_COMPACT_ALLOCATION
    st_thread->next = &l->nodes[pid];
#else
    st_thread->next = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CCSynchStruct));
#endif
}
