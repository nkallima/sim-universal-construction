#include <hsynch.h>

#ifdef NUMA_SUPPORT
#   include <numa.h>
#endif

#define HSYNCH_HELP_FACTOR                    10
#define HSYNCH_DEFAULT_NUMA_NODE_SIZE         8

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
     while (counter < help_bound && p->next != null) {
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

void HSynchThreadStateInit(HSynchStruct *l, HSynchThreadState *st_thread, int pid) {
    st_thread->next_node = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HSynchNode));

    
#ifdef NUMA_SUPPORT
    if (l->numa_policy) {
        if (getPreferedCore() != -1) {
            node_of_thread = numa_node_of_cpu(getPreferedCore());
            if (node_of_thread == -1) 
                node_of_thread = pid / l->numa_node_size;
        }
    } else {
        int ncpus = numa_num_configured_cpus();
        int hw_numa_size = (ncpus/2)/l->numa_nodes + (((ncpus/2) % l->numa_nodes) == 0 ? 0 : 1);

        if (numa_node_of_cpu(0) == numa_node_of_cpu(ncpus/2)) {
            if (getPreferedCore() >= ncpus/2)
                node_of_thread = (getPreferedCore() - ncpus/2) / hw_numa_size;
            else
                node_of_thread = getPreferedCore() / hw_numa_size;
        } else {
            node_of_thread = pid / l->numa_node_size;
        }
        //fprintf(stderr, "### thread pid: %d -- getPreferedCore: %d -- lock_node: %d -- hw_numa_size: %d\n", pid, getPreferedCore(), node_of_thread, hw_numa_size);
    }
#else
    node_of_thread = pid / l->numa_node_size;
#endif
}

void HSynchStructInit(HSynchStruct *l, uint32_t nthreads, uint32_t numa_regions) {
    int i;

    l->nthreads = nthreads;
    if (numa_regions == HSYNCH_DEFAULT_NUMA_POLICY) {
        l->numa_policy = true;
#ifdef NUMA_SUPPORT
        l->numa_nodes = numa_max_node() + 1;
        l->numa_node_size = nthreads/l->numa_nodes + ((nthreads%l->numa_nodes > 0) ? 1 : 0);
#else
        l->numa_node_size = HSYNCH_DEFAULT_NUMA_NODE_SIZE;
        l->numa_nodes = nthreads/l->numa_node_size + (nthreads%l->numa_node_size == 0 ? 0 : 1);
        if (l->numa_nodes == 0) l->numa_nodes = 1;
#endif
    } else {
        l->numa_policy = false;
        l->numa_nodes = numa_regions;
        l->numa_node_size = nthreads/l->numa_nodes + ((nthreads%l->numa_nodes > 0) ? 1 : 0);
    }

    l->central_lock = CLHLockInit(nthreads);
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, l->numa_nodes * sizeof(HSynchNodePtr));
    for (i = 0; i < l->numa_nodes; i++) {
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
