#include <ccsynch.h>

static const int CCSYNCH_HELP_FACTOR = 10;


RetVal CCSynchApplyOp(CCSynchStruct *l, CCSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile CCSynchNode *p;
    volatile CCSynchNode *cur;
    CCSynchNode *next_node, *tmp_next;
    int help_bound = CCSYNCH_HELP_FACTOR * l->nthreads;
    int counter = 0;

    next_node = st_thread->next;
    next_node->next = null;
    next_node->locked = true;
    next_node->completed = false;

    cur = (CCSynchNode *)SWAP(&l->Tail, next_node);
    cur->arg_ret = arg;
    cur->next = (CCSynchNode *)next_node;
    st_thread->next = (CCSynchNode *)cur;

    while (cur->locked) {                   // spinning
            resched();
    }
    if (cur->completed)                     // I have been helped
        return cur->arg_ret;
#ifdef DEBUG
    l->rounds++;
    l->combiner_counter[pid] += 1;
#endif
    p = cur;                                // I am not been helped
    while (p->next != null && counter < help_bound) {
        StorePrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        tmp_next = p->next;
        p->arg_ret = sfunc(state, p->arg_ret, p->pid);
        p->completed = true;
        p->locked = false;
        p = tmp_next;
    }
    p->locked = false;                      // Unlock the next one
    StoreFence();

    return cur->arg_ret;
}

void CCSynchStructInit(CCSynchStruct *l, uint32_t nthreads) {
    l->nthreads = nthreads;
#ifdef DEBUG
    int i;

    l->rounds = l->counter =0;
    l->combiner_counter = getAlignedMemory(CACHE_LINE_SIZE, sizeof(int));
    for (i=0; i < nthreads; i++)
        l->combiner_counter[i] += 1;
#endif
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CCSynchNode));
    l->Tail->next = null;
    l->Tail->locked = false;
    l->Tail->completed = false;

    StoreFence();
}

void CCSynchThreadStateInit(CCSynchThreadState *st_thread, int pid) {
    st_thread->next = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CCSynchNode));
}
