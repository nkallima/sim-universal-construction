#include <hsynch.h>

#ifdef NUMA_SUPPORT
#   include <numa.h>
#endif


static const int HSYNCH_HELP_FACTOR = 10;
static int HSYNCH_CLUSTER_SIZE = 8;

static __thread int node_of_thread = 0;

RetVal HSynchApplyOp(HSynchStruct *l, HSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile HSynchNode *p;
    volatile HSynchNode *cur;
    register HSynchNode *next_node, *tmp_next;
    register int counter = 0;
    int help_bound = HSYNCH_HELP_FACTOR * l->nthreads;

    next_node = st_thread->next_node;
    next_node->next = null;
    next_node->locked = true;
    next_node->completed = false;

    cur = (volatile HSynchNode *)SWAP(&l->Tail[node_of_thread].ptr, next_node);
    cur->arg_ret = arg;
    cur->next = (HSynchNode *)next_node;

    st_thread->next_node = (HSynchNode *)cur;

    while (cur->locked)                     // spinning
        resched();

    p = cur;                                // I am not been helped
    if (cur->completed)                     // I have been helped
        return cur->arg_ret;
    CLHLock(l->central_lock, pid);
#ifdef DEBUG
    l->rounds++;
#endif
    while (p->next != null && counter < help_bound) {
        ReadPrefetch(p->next);
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
    CLHUnlock(l->central_lock, pid);

    return cur->arg_ret;
}

void HSynchThreadStateInit(HSynchThreadState *st_thread, int pid) {
    st_thread->next_node = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HSynchNode));

#ifdef NUMA_SUPPORT
    if (getPreferedCore() != -1)
        node_of_thread = numa_node_of_cpu(getPreferedCore());
#else
    node_of_thread = pid / HSYNCH_CLUSTER_SIZE;
#endif

}

void HSynchStructInit(HSynchStruct *l, uint32_t nthreads) {
    int numa_regions = 1, i;

    l->nthreads = nthreads;
#ifdef NUMA_SUPPORT
    numa_regions = numa_max_node() + 1;
    HSYNCH_CLUSTER_SIZE = nthreads/numa_regions + ((nthreads%numa_regions > 0) ? 1 : 0);
#else 
    numa_regions = nthreads/HSYNCH_CLUSTER_SIZE + (nthreads%HSYNCH_CLUSTER_SIZE == 0 ? 0 : 1);
    if (numa_regions == 0) numa_regions = 1;
#endif
    
    l->central_lock = CLHLockInit(nthreads);
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, numa_regions * sizeof(HSynchNodePtr));
    for (i = 0; i < numa_regions; i++) {
        l->Tail[i].ptr = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HSynchNode));
        l->Tail[i].ptr->next = null;
        l->Tail[i].ptr->locked = false;
        l->Tail[i].ptr->completed = false;
    }
#ifdef DEBUG
    l->rounds = l->counter =0;
#endif
    StoreFence();
}
