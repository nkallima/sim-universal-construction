#include <dsmsynch.h>

static const int DSMSYNCH_HELP_FACTOR = 10;

RetVal DSMSynchApplyOp(DSMSynchStruct *l, DSMSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile DSMSynchNode *mynode;
    DSMSynchNode *mypred;
    volatile DSMSynchNode *p;
    register int counter;
    int help_bound = DSMSYNCH_HELP_FACTOR * l->nthreads;

    st_thread->toggle = 1 - st_thread->toggle;
    mynode = st_thread->MyNodes[st_thread->toggle];

    mynode->next = NULL;
    mynode->arg_ret = arg;
    mynode->pid = pid;
    mynode->locked = true;
    mynode->completed = false;
    mynode->pid = pid;

    mypred = (DSMSynchNode *)SWAP(&l->Tail, mynode);
    if (mypred != NULL) {
        mypred->next = (DSMSynchNode *)mynode;
        FullFence();

        while (mynode->locked) {
            resched();
        }
        if (mynode->completed) // operation has already applied
            return mynode->arg_ret;
    }

#ifdef DEBUG
    l->rounds += 1;
#endif
    counter = 0;
    p = mynode;
    do { // I surely do it for myself
        ReadPrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter += 1;
#endif
        p->arg_ret = sfunc(state, p->arg_ret, p->pid);
        NonTSOFence();
        p->completed = true;
        NonTSOFence();
        p->locked = false;
        if (p->next == NULL || p->next->next == NULL || counter >= help_bound)
            break;
        p = p->next;
    } while (true);
    // End critical section
    if (p->next == NULL) {
        if (l->Tail == p && CASPTR(&l->Tail, p, NULL) == true)
            return mynode->arg_ret;
        while (p->next == NULL) {
            resched();
        }
    }
    NonTSOFence();
    p->next->locked = false;
    FullFence();

    return mynode->arg_ret;
}

void DSMSynchStructInit(DSMSynchStruct *l, uint32_t nthreads) {
    l->nthreads = nthreads;
    l->Tail = NULL;

#ifdef SYNCH_COMPACT_ALLOCATION
    l->nodes = getAlignedMemory(CACHE_LINE_SIZE, 2 * nthreads * sizeof(DSMSynchNode));
#else
    l->nodes = NULL;
#endif

#ifdef DEBUG
    l->counter = 0;
#endif
    FullFence();
}

void DSMSynchThreadStateInit(DSMSynchStruct *l, DSMSynchThreadState *st_thread, int pid) {
#ifdef SYNCH_COMPACT_ALLOCATION
    st_thread->MyNodes[0] = &l->nodes[2 * pid];
    st_thread->MyNodes[1] = &l->nodes[2 * pid + 1];
#else
    DSMSynchNode *nodes = getAlignedMemory(CACHE_LINE_SIZE, 2 * sizeof(DSMSynchNode));
    st_thread->MyNodes[0] = &nodes[0];
    st_thread->MyNodes[1] = &nodes[1];
#endif

    st_thread->toggle = 0;
}
