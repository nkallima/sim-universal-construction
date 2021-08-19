#include <osci.h>
#include <threadtools.h>

static const int OSCI_HELP_FACTOR = 10;

void OsciThreadStateInit(OsciThreadState *st_thread, OsciStruct *l, int pid) {
    int i, j;

    st_thread->toggle = 0;
    for (i = 0; i < 2; i++) {
        st_thread->next_node[i].rec = synchGetMemory(l->fibers_per_thread * sizeof(OsciFiberRec));
        for (j = 0; j < l->fibers_per_thread; j++) {
            st_thread->next_node[i].rec[j].arg_ret = 0;
            st_thread->next_node[i].rec[j].pid = -1;
            st_thread->next_node[i].rec[j].completed = true;
            st_thread->next_node[i].rec[j].locked = true;
        }
        st_thread->next_node[i].next = NULL;
        st_thread->next_node[i].toggle = 0;
        st_thread->next_node[i].door = _OSCI_DOOR_INIT;
    }
}

RetVal OsciApplyOp(OsciStruct *l, OsciThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile OsciNode *p, *pred, *cur, *mynode;
    int counter = 0, i;
    int help_bound = OSCI_HELP_FACTOR * l->nthreads;
    int group = pid / l->fibers_per_thread;
    int offset_id = pid % l->fibers_per_thread;

    mynode = &st_thread->next_node[st_thread->toggle];
osci_start:
    do { // Try to acquire the combining point
        if (l->current_node[group].ptr == NULL)
            CASPTR(&l->current_node[group].ptr, NULL, mynode);
        cur = l->current_node[group].ptr;
    } while (cur == NULL);

    if (cur == mynode) { // In that case, I'm the combiner
        st_thread->toggle = 1 - st_thread->toggle;
        cur->rec[offset_id].arg_ret = arg;
        cur->rec[offset_id].pid = pid;
        cur->rec[offset_id].locked = true;
        cur->rec[offset_id].completed = false;
        cur->next = NULL;
        cur->door = _OSCI_DOOR_OPENED;
        synchResched();                    // Scheduling point
        l->current_node[group].ptr = NULL; // Release the combining point
        while (!CAS32(&cur->door, _OSCI_DOOR_OPENED, _OSCI_DOOR_INIT))
            synchResched();
        pred = SWAP(&l->Tail, cur);

        if (pred != NULL) {
            pred->next = cur;
            FullFence();
            while (cur->rec[offset_id].locked)
                synchResched();
            if (cur->rec[offset_id].completed) // operation has already applied
                return cur->rec[offset_id].arg_ret;
        }
    } else {
        while (!CAS32(&cur->door, _OSCI_DOOR_OPENED, _OSCI_DOOR_LOCKED)) {
            if (cur->door == _OSCI_DOOR_INIT)
                goto osci_start;
            synchResched();
        }
        cur->rec[offset_id].arg_ret = arg;
        cur->rec[offset_id].pid = pid;
        cur->rec[offset_id].locked = true;
        cur->rec[offset_id].completed = false;
        cur->door = _OSCI_DOOR_OPENED;
        do {
            synchResched();
        } while (cur->rec[offset_id].locked);
        if (cur->rec[offset_id].completed) // I have been helped
            return cur->rec[offset_id].arg_ret;
    }

#ifdef DEBUG
    l->rounds++;
#endif
    p = cur;
    do {
        StorePrefetch(p->next);
        for (i = 0; i < l->fibers_per_thread; i++) {
            if (p->rec[i].completed == false) {
                p->rec[i].arg_ret = sfunc(state, p->rec[i].arg_ret, p->rec[i].pid);
                p->rec[i].completed = true;
                p->rec[i].locked = false;
#ifdef DEBUG
                l->counter += 1;
#endif
            }
        }
        counter += i;
        if (p->next == NULL || p->next->next == NULL || counter >= help_bound)
            break;
        p = p->next;
    } while (true);
    // End critical section
    if (p->next == NULL) {
        if (l->Tail == p && CASPTR(&l->Tail, p, NULL) == true)
            return cur->rec[offset_id].arg_ret;
        while (p->next == NULL) {
            synchResched();
        }
    }
    NonTSOFence();
    i = 0;
    while (i < l->fibers_per_thread) {
        if (p->next->rec[i].completed == false) {
            p->next->rec[i].locked = false; // Unlock the next one
            break;
        }
        i++;
    }
    FullFence();

    return cur->rec[offset_id].arg_ret;
}

void OsciInit(OsciStruct *l, uint32_t nthreads, uint32_t fibers_per_thread) {
    int i;

    l->nthreads = nthreads;
    if (fibers_per_thread <= nthreads)
        l->fibers_per_thread = fibers_per_thread;
    else
        l->fibers_per_thread = 1;
    l->groups_of_fibers = (nthreads / fibers_per_thread) + (nthreads % fibers_per_thread != 0 ? 1 : 0);
#ifdef DEBUG
    l->rounds = l->counter = 0;
#endif
    l->current_node = synchGetAlignedMemory(CACHE_LINE_SIZE, l->groups_of_fibers * sizeof(ptr_aligned_t));
    for (i = 0; i < l->groups_of_fibers; i++)
        l->current_node[i].ptr = NULL;
    l->Tail = NULL;
    StoreFence();
}
