#include <sim.h>
#include <simqueue.h>

static const int GUARD_VALUE = INT_MAX;
static const int LOCAL_POOL_SIZE = _SIM_LOCAL_POOL_SIZE_;

inline static void connectQueue(SimQueueStruct *queue);;

void SimQueueThreadStateInit(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid) {
    TVEC_SET_ZERO(&th_state->mask);
    TVEC_SET_ZERO(&th_state->my_enq_bit);
    TVEC_SET_ZERO(&th_state->enq_toggle);
    TVEC_REVERSE_BIT(&th_state->my_enq_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    th_state->enq_toggle = TVEC_NEGATIVE(th_state->mask);
    init_pool(&th_state->pool_node, sizeof(Node));

    TVEC_SET_ZERO(&th_state->mask);
    TVEC_SET_ZERO(&th_state->my_deq_bit);
    TVEC_SET_ZERO(&th_state->deq_toggle);
    TVEC_REVERSE_BIT(&th_state->my_deq_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    th_state->deq_toggle = TVEC_NEGATIVE(th_state->mask);
    th_state->deq_local_index = 0;
    th_state->enq_local_index = 0;
    th_state->backoff = 1;
    th_state->mybank = TVEC_GET_BANK_OF_BIT(pid);
}

void SimQueueInit(SimQueueStruct *queue, int max_backoff) {
    pointer_t tmp_sp;

    tmp_sp.struct_data.index = LOCAL_POOL_SIZE * N_THREADS;
    tmp_sp.struct_data.seq = 0L;
    queue->enq_sp = tmp_sp;

    tmp_sp.struct_data.index = LOCAL_POOL_SIZE * N_THREADS;
    tmp_sp.struct_data.seq = 0L;
    queue->deq_sp = tmp_sp;

    TVEC_SET_ZERO((ToggleVector *)&queue->enqueuers);
    TVEC_SET_ZERO((ToggleVector *)&queue->dequeuers);

    // Initializing queue's state
    // --------------------------
    TVEC_SET_ZERO((ToggleVector *) &queue->enq_pool[LOCAL_POOL_SIZE * N_THREADS].applied);
    queue->enq_pool[LOCAL_POOL_SIZE * N_THREADS].link_a = &queue->guard;
    queue->enq_pool[LOCAL_POOL_SIZE * N_THREADS].link_b = null;
    queue->enq_pool[LOCAL_POOL_SIZE * N_THREADS].ptr = &queue->guard;
    TVEC_SET_ZERO((ToggleVector *) &queue->deq_pool[LOCAL_POOL_SIZE * N_THREADS].applied);
    queue->deq_pool[LOCAL_POOL_SIZE * N_THREADS].ptr = &queue->guard;
#ifdef DEBUG
    queue->enq_pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0L;
    queue->deq_pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0L;
#endif
    queue->MAX_BACK = max_backoff * 100;
	queue->guard.val = GUARD_VALUE;
	queue->guard.next = null;
    FullFence();
}


inline static void connectQueue(SimQueueStruct *queue) {
    EnqState lsp_data;
    pointer_t tmp_sp;

    LoadFence();
    tmp_sp = queue->enq_sp;
    lsp_data = queue->enq_pool[tmp_sp.struct_data.index];
    CASPTR(&lsp_data.link_a->next, null, lsp_data.link_b);
}

void SimQueueEnqueue(SimQueueStruct *queue, SimQueueThreadState *th_state, ArgVal arg, int pid) {
    ToggleVector diffs, l_toggles;
    pointer_t ldw, mod_dw, tmp_sp;
    int i, j, enq_counter, prefix;
    EnqState *mod_sp, *lsp_data;
    Node *node, *llist;

    queue->announce[pid] = arg; // A Fetch&Add instruction follows soon, thus a barrier is needless
    TVEC_REVERSE_BIT(&th_state->my_enq_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->enq_toggle, &th_state->enq_toggle, th_state->mybank);
    mod_sp = &queue->enq_pool[pid * LOCAL_POOL_SIZE + th_state->enq_local_index];
    TVEC_ATOMIC_ADD_BANK(&queue->enqueuers, &th_state->enq_toggle, th_state->mybank);            // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
#if N_THREADS > USE_CPUS
    if (fastRandomRange(1, N_THREADS) > 4)
        resched();
#else
    volatile int k;
    int backoff_limit;

    if (fastRandomRange(1, N_THREADS) > 1) {
        backoff_limit =  fastRandomRange(th_state->backoff >> 1, th_state->backoff);
        for (k = 0; k < backoff_limit; k++)
            ;
    }
#endif

    for (j = 0; j < 2; j++) {
        tmp_sp = queue->enq_sp;          // This is an atomic read, since sp is volatile
        ldw = *((pointer_t *) & tmp_sp);
        lsp_data = &queue->enq_pool[ldw.struct_data.index];
        TVEC_ATOMIC_COPY_BANKS(&mod_sp->applied, &lsp_data->applied, th_state->mybank);
        TVEC_AND_BANKS(&diffs, &mod_sp->applied, &th_state->mask, th_state->mybank);
        diffs = TVEC_XOR(diffs, th_state->my_enq_bit);   // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                // if the operation has already been applied return
            return;
        *mod_sp = *lsp_data;
        l_toggles = queue->enqueuers;    // This is an atomic read, since sp is volatile
        if (tmp_sp.raw_data != queue->enq_sp.raw_data)
            continue;
        diffs = TVEC_XOR(mod_sp->applied, l_toggles);
        if (mod_sp->link_a->next == null)  // avoid owned state (MOESI protocol)
            CASPTR(&mod_sp->link_a->next, null, mod_sp->link_b);
        enq_counter = 1;
        node = alloc_obj(&th_state->pool_node);
        node->next = null;
        node->val = arg;
        llist = node;
        TVEC_REVERSE_BIT(&diffs, pid);
#ifdef DEBUG
        mod_sp->counter += 1;
#endif
        enq_counter = 0;
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                enq_counter++;
#ifdef DEBUG
                mod_sp->counter += 1;
#endif
                node->next = alloc_obj(&th_state->pool_node);
                node = (Node *)node->next;
                node->next = null;
                node->val = queue->announce[proc_id];
                diffs.cell[i] ^= 1L << pos;
            }
        }

        mod_sp->link_a = mod_sp->ptr;
        mod_sp->link_b = llist;
        mod_sp->ptr = node;
        mod_sp->applied = l_toggles;
        mod_dw.struct_data.seq = ldw.struct_data.seq + 1;
        mod_dw.struct_data.index = LOCAL_POOL_SIZE * pid + th_state->enq_local_index;
        if (tmp_sp.raw_data == queue->enq_sp.raw_data &&
            CAS64(&queue->enq_sp, tmp_sp.raw_data, mod_dw.raw_data)) {
            CASPTR(&mod_sp->link_a->next, null, mod_sp->link_b);
            th_state->enq_local_index = (th_state->enq_local_index + 1) % LOCAL_POOL_SIZE;
            th_state->backoff = (th_state->backoff >> 1) | 1;
            return;
        }
        else {
            if (th_state->backoff < queue->MAX_BACK) th_state->backoff <<= 1;
            rollback(&th_state->pool_node, enq_counter);
        }
    }
    return;
}


RetVal SimQueueDequeue(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid) {
    ToggleVector diffs, l_toggles;
    DeqState *mod_sp, *lsp_data;
    pointer_t tmp_sp;
    int i, j, prefix;
    pointer_t ldw, mod_dw;
    volatile Node *next;

    TVEC_REVERSE_BIT(&th_state->my_deq_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->deq_toggle, &th_state->deq_toggle, th_state->mybank);
    mod_sp = &queue->deq_pool[pid * LOCAL_POOL_SIZE + th_state->deq_local_index];
    TVEC_ATOMIC_ADD_BANK(&queue->dequeuers, &th_state->deq_toggle, th_state->mybank);            // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
#if N_THREADS > USE_CPUS
    if (fastRandomRange(1, N_THREADS) > 4)
       resched();
#else
    volatile int k;
    int backoff_limit;

    if (fastRandomRange(1, N_THREADS) > 1) {
        backoff_limit =  fastRandomRange(th_state->backoff >> 1, th_state->backoff);
        for (k = 0; k < backoff_limit; k++)
            ;
    }
#endif

    for (j = 0; j < 2; j++) {
        tmp_sp = queue->deq_sp;                                 // This is an atomic read, since sp is volatile
        ldw = *((pointer_t *) & tmp_sp);
        lsp_data = &queue->deq_pool[ldw.struct_data.index];
        TVEC_ATOMIC_COPY_BANKS(&mod_sp->applied, &lsp_data->applied, th_state->mybank);
        TVEC_AND_BANKS(&diffs, &mod_sp->applied, &th_state->mask, th_state->mybank);
        diffs = TVEC_XOR(diffs, th_state->my_deq_bit);         // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                           // if the operation has already been applied return
            break;
        *mod_sp = *lsp_data;
        l_toggles = queue->dequeuers;                         // This is an atomic read, since sp is volatile
        if (tmp_sp.raw_data != queue->deq_sp.raw_data)
            continue;
        diffs = TVEC_XOR(mod_sp->applied, l_toggles);
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                next = mod_sp->ptr->next;
#ifdef DEBUG
                mod_sp->counter += 1;
#endif
                if (next != null) {
                    mod_sp->ret[proc_id] = next->val;
                    mod_sp->ptr = (Node *)next;
                } else {
                    connectQueue(queue);
                    next = mod_sp->ptr->next;
                    if (next == null)
                        mod_sp->ret[proc_id] = GUARD_VALUE;
                    else {
                        mod_sp->ret[proc_id] = next->val;
                        mod_sp->ptr = (Node *)next;
                    }
                }
                diffs.cell[i] ^= 1L << pos;
            }
        }
        mod_sp->applied = l_toggles;
        mod_dw.struct_data.seq = ldw.struct_data.seq + 1;
        mod_dw.struct_data.index = LOCAL_POOL_SIZE * pid + th_state->deq_local_index;
        if (tmp_sp.raw_data == queue->deq_sp.raw_data && 
            CAS64(&queue->deq_sp, ldw.raw_data, mod_dw.raw_data)) {
            th_state->deq_local_index = (th_state->deq_local_index + 1) % LOCAL_POOL_SIZE;
            th_state->backoff = (th_state->backoff >> 1) | 1;
            return mod_sp->ret[pid];
        } else if (th_state->backoff < queue->MAX_BACK) th_state->backoff <<= 1;
    }

    LoadFence();
    tmp_sp = queue->deq_sp;
    return queue->deq_pool[tmp_sp.struct_data.index].ret[pid];
}
