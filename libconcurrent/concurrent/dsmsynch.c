#include <dsmsynch.h>


static const int DSMSIM_HELP_BOUND = 10 * N_THREADS;

RetVal DSMSynchApplyOp(DSMSynchStruct *l, DSMSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile DSMSynchNode *mynode;
    DSMSynchNode *mypred;
    volatile DSMSynchNode *p;
    register int counter;

    st_thread->toggle = 1 - st_thread->toggle;
    mynode = st_thread->MyNodes[st_thread->toggle];
    
    mynode->next = null;
    mynode->arg = arg;
    mynode->ret = 0;
    mynode->locked = true;
    mynode->completed = false;
    mynode->pid = pid;
#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
    mypred = (DSMSynchNode *)SWAP(&l->Tail, mynode);
    if (mypred != null) {
        mypred->next = (DSMSynchNode *)mynode;
        FullFence();
#if defined(__sun) || defined(sun)
        schedctl_stop(st_thread->schedule_control);
#endif
        while (mynode->locked) {
#if N_THREADS > USE_CPUS
            resched();
#elif defined(sparc)
            Pause();
            Pause();
            Pause();
            Pause();
#else
            Pause();
            Pause();
#endif
        }
        if (mynode->completed) // operation has already applied
            return mynode->ret;
    }
#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
#ifdef DEBUG
    l->rounds += 1;
#endif
    counter = 0;
    p = mynode;
    do {  // I surely do it for myself
        ReadPrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter += 1;
#endif
        p->ret = sfunc(state, p->arg, p->pid);
        p->completed = true;
        p->locked = false;
        if (p->next == null || p->next->next == null || counter >= DSMSIM_HELP_BOUND)
            break;
        p = p->next;
    } while(true);
    // End critical section
    if (p->next == null) {
        if (l->Tail == p && CASPTR(&l->Tail, p, null) == true)
            return mynode->ret;
        while (p->next == null) {
            resched();
        }
    }
    p->next->locked = false;
    FullFence();
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif
    return mynode->ret;
}

void DSMSynchStructInit(DSMSynchStruct *l) {
    l->Tail = null;
#ifdef DEBUG
    l->counter = 0;
#endif
    FullFence();
}

void DSMSynchThreadStateInit(DSMSynchThreadState *st_thread, int pid) {
    st_thread->MyNodes[0] = getAlignedMemory(CACHE_LINE_SIZE, sizeof(DSMSynchNode));
    st_thread->MyNodes[1] = getAlignedMemory(CACHE_LINE_SIZE, sizeof(DSMSynchNode));
    st_thread->toggle = 0;
#ifdef sun
    st_thread->schedule_control = schedctl_init();
#endif
}
