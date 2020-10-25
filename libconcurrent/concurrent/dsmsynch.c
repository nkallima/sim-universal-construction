#include <dsmsynch.h>


static const int DSMSIM_HELP_FACTOR = 10;

RetVal DSMSynchApplyOp(DSMSynchStruct *l, DSMSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile DSMSynchNode *mynode;
    DSMSynchNode *mypred;
    volatile DSMSynchNode *p;
    register int counter;
    int help_bound = DSMSIM_HELP_FACTOR * l->nthreads;

    st_thread->toggle = 1 - st_thread->toggle;
    mynode = st_thread->MyNodes[st_thread->toggle];
    
    mynode->next = null;
    mynode->arg = arg;
    mynode->ret = 0;
    mynode->locked = true;
    mynode->completed = false;
    mynode->pid = pid;

    mypred = (DSMSynchNode *)SWAP(&l->Tail, mynode);
    if (mypred != null) {
        mypred->next = (DSMSynchNode *)mynode;
        FullFence();

        while (mynode->locked) {
            resched();
        }
        if (mynode->completed) // operation has already applied
            return mynode->ret;
    }

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
        if (p->next == null || p->next->next == null || counter >= help_bound)
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

    return mynode->ret;
}

void DSMSynchStructInit(DSMSynchStruct *l, uint32_t nthreads) {
    l->nthreads = nthreads;
    l->nodes = getAlignedMemory(CACHE_LINE_SIZE, 2 * nthreads * sizeof(DSMSynchNode));
    l->Tail = null;
#ifdef DEBUG
    l->counter = 0;
#endif
    FullFence();
}

void DSMSynchThreadStateInit(DSMSynchStruct *l, DSMSynchThreadState *st_thread, int pid) {
    st_thread->MyNodes[0] = &l->nodes[pid];
    st_thread->MyNodes[1] = &l->nodes[l->nthreads + pid];
    st_thread->toggle = 0;
}
