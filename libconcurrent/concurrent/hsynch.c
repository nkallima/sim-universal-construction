#include <hsynch.h>

#ifdef NUMA_SUPPORT
#   include <numa.h>
#endif


static const int HSYNCH_HELP_BOUND = 10 * N_THREADS;
static int HSYNCH_CLUSTER_SIZE = 1;

RetVal HSynchApplyOp(HSynchStruct *l, HSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile HSynchNode *p;
    volatile HSynchNode *cur;
    register HSynchNode *next_node, *tmp_next;
    register int counter = 0;

    next_node = st_thread->next_node;
    next_node->next = null;
    next_node->locked = true;
    next_node->completed = false;

#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
    cur = (volatile HSynchNode *)SWAP(&l->Tail[pid / HSYNCH_CLUSTER_SIZE].ptr, next_node);
    cur->arg_ret = arg;
    cur->next = (HSynchNode *)next_node;
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif
    st_thread->next_node = (HSynchNode *)cur;

    while (cur->locked) {                   // spinning
#if N_THREADS > USE_CPUS
        resched();
#elif defined(sparc)
        Pause();
        Pause();
        Pause();
        Pause();
#else
        ;
#endif
    }
#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
    p = cur;                                // I am not been helped
    if (cur->completed)                     // I have been helped
        return cur->arg_ret;
    CLHLock(l->central_lock, pid);
#ifdef DEBUG
    l->rounds++;
#endif
    while (p->next != null && counter < HSYNCH_HELP_BOUND) {
        ReadPrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        tmp_next = p->next;
        p->arg_ret = sfunc(state, p->arg_ret, p->pid);
        p->completed = true;
        p->locked = false;
        WeakFence();
        p = tmp_next;
    }
    p->locked = false;                      // Unlock the next one
    CLHUnlock(l->central_lock, pid);
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif

    return cur->arg_ret;
}

void HSynchThreadStateInit(HSynchThreadState *st_thread, int pid) {
    st_thread->next_node = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HSynchNode));
#ifdef sun
    st_thread->schedule_control = schedctl_init();
#endif
}

void HSynchStructInit(HSynchStruct *l) {
    int numa_regions = 1, i;

#ifdef NUMA_SUPPORT
    numa_regions = numa_max_node();
    // The next line is only usefull for non-NUMA machines
    if (numa_regions == 0) numa_regions = 1;
#endif

    HSYNCH_CLUSTER_SIZE = N_THREADS/numa_regions + N_THREADS%numa_regions;
    l->central_lock = CLHLockInit();
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
