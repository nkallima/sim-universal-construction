#include <simqueue.h>
#include <fastrand.h>
#include <threadtools.h>

static const int LOCAL_POOL_SIZE = _SIM_LOCAL_POOL_SIZE_;

static void EnqStateCopy(EnqState *dest, EnqState *src);
static void DeqStateCopy(DeqState *dest, DeqState *src);
static void EnqLinkQueue(SimQueueStruct *queue, EnqState *pst);
static void DeqLinkQueue(SimQueueStruct *queue, DeqState *pst);

static void EnqLinkQueue(SimQueueStruct *queue, EnqState *pst) {
    if (pst->first != NULL) {
        synchCASPTR(&pst->first->next, NULL, pst->last);
    }
}

static void DeqLinkQueue(SimQueueStruct *queue, DeqState *pst) {
    pointer_t enq_sp;
    EnqState *enq_pst;

    enq_sp.raw_data = queue->enq_sp.raw_data;
    enq_pst = queue->enq_pool[enq_sp.struct_data.index];

    if (pst->head->next == NULL) {
        volatile Node *last = enq_pst->last;
        volatile Node *first = enq_pst->first;
        synchFullFence();
        if (first != NULL && last != NULL && enq_sp.raw_data == queue->enq_sp.raw_data) synchCASPTR(&first->next, NULL, last);
    }
}

static void EnqStateCopy(EnqState *dest, EnqState *src) {
    // copy everything except 'applied'
    memcpy(&dest->copy_point, &src->copy_point,
           EnqStateSize(src->applied.nthreads) - sizeof(ToggleVector));
}

static void DeqStateCopy(DeqState *dest, DeqState *src) {
    // copy everything except 'applied' and 'ret' fields
    memcpy(&dest->copy_point, &src->copy_point,
           DeqStateSize(src->applied.nthreads)- sizeof(ToggleVector) - sizeof(RetVal *));
}

void SimQueueThreadStateInit(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid) {
    TVEC_INIT(&th_state->mask, queue->nthreads);
    TVEC_INIT(&th_state->deq_toggle, queue->nthreads);
    TVEC_INIT(&th_state->my_deq_bit, queue->nthreads);
    TVEC_INIT(&th_state->enq_toggle, queue->nthreads);
    TVEC_INIT(&th_state->my_enq_bit, queue->nthreads);
    TVEC_INIT(&th_state->diffs, queue->nthreads);
    TVEC_INIT(&th_state->l_toggles, queue->nthreads);

    TVEC_REVERSE_BIT(&th_state->my_enq_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    TVEC_NEGATIVE(&th_state->enq_toggle, &th_state->mask);
    synchInitPool(&th_state->pool_node, sizeof(Node));

    TVEC_SET_ZERO(&th_state->mask);
    TVEC_REVERSE_BIT(&th_state->my_deq_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    TVEC_NEGATIVE(&th_state->deq_toggle, &th_state->mask);
    th_state->deq_local_index = 0;
    th_state->enq_local_index = 0;
    th_state->max_backoff = 1;
}

void SimQueueStructInit(SimQueueStruct *queue, uint32_t nthreads, int max_backoff) {
    pointer_t tmp_sp;
    int i;

    queue->nthreads = nthreads;
    queue->announce = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(ArgVal));
    TVEC_INIT_AT(&queue->enqueuers, nthreads, synchGetAlignedMemory(CACHE_LINE_SIZE, _TVEC_VECTOR_SIZE(nthreads)));
    TVEC_INIT_AT(&queue->dequeuers, nthreads, synchGetAlignedMemory(CACHE_LINE_SIZE, _TVEC_VECTOR_SIZE(nthreads)));

    tmp_sp.struct_data.index = LOCAL_POOL_SIZE * nthreads;
    tmp_sp.struct_data.seq = 0L;
    queue->enq_sp = tmp_sp;
    queue->deq_sp = tmp_sp;

    TVEC_SET_ZERO((ToggleVector *)&queue->enqueuers);
    TVEC_SET_ZERO((ToggleVector *)&queue->dequeuers);

    queue->enq_pool = synchGetAlignedMemory(CACHE_LINE_SIZE, (LOCAL_POOL_SIZE * nthreads + 1) * sizeof(EnqState *));
    queue->deq_pool = synchGetAlignedMemory(CACHE_LINE_SIZE, (LOCAL_POOL_SIZE * nthreads + 1) * sizeof(DeqState *));

    for (i = 0; i < LOCAL_POOL_SIZE * nthreads + 1; i++) {
        queue->enq_pool[i] = synchGetAlignedMemory(CACHE_LINE_SIZE, EnqStateSize(nthreads));
        queue->deq_pool[i] = synchGetAlignedMemory(CACHE_LINE_SIZE, DeqStateSize(nthreads));

        TVEC_INIT_AT(&queue->enq_pool[i]->applied, nthreads, queue->enq_pool[i]->__flex);
        TVEC_INIT_AT(&queue->deq_pool[i]->applied, nthreads, queue->deq_pool[i]->__flex);

        queue->deq_pool[i]->ret = ((void *)queue->deq_pool[i]->__flex) + _TVEC_VECTOR_SIZE(nthreads);
    }

    // Initializing queue's state
    // --------------------------
    queue->guard.val = GUARD_VALUE;
    queue->guard.next = NULL;
    TVEC_SET_ZERO((ToggleVector *)&queue->enq_pool[LOCAL_POOL_SIZE * nthreads]->applied);
    queue->enq_pool[LOCAL_POOL_SIZE * nthreads]->tail = &queue->guard;
    queue->enq_pool[LOCAL_POOL_SIZE * nthreads]->first = NULL;
    queue->enq_pool[LOCAL_POOL_SIZE * nthreads]->last = NULL;
    TVEC_SET_ZERO((ToggleVector *)&queue->deq_pool[LOCAL_POOL_SIZE * nthreads]->applied);
    queue->deq_pool[LOCAL_POOL_SIZE * nthreads]->head = &queue->guard;
#ifdef DEBUG
    queue->enq_pool[LOCAL_POOL_SIZE * nthreads]->counter = 0L;
    queue->deq_pool[LOCAL_POOL_SIZE * nthreads]->counter = 0L;
#endif
    queue->MAX_BACK = max_backoff * 100;

    synchFullFence();
}

void SimQueueEnqueue(SimQueueStruct *queue, SimQueueThreadState *th_state, ArgVal arg, int pid) {
    ToggleVector *diffs = &th_state->diffs,
                 *l_toggles = &th_state->l_toggles;
    pointer_t old_sp, new_sp;
    int i, j, enq_counter, prefix;
    EnqState *lsp_data, *sp_data;
    Node *node, *llist;

    int mybank = TVEC_GET_BANK_OF_BIT(pid, queue->nthreads);
    queue->announce[pid] = arg; // A Fetch&Add instruction follows soon, thus a barrier is needless
    TVEC_REVERSE_BIT(&th_state->my_enq_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->enq_toggle, &th_state->enq_toggle, mybank);
    lsp_data = queue->enq_pool[pid * LOCAL_POOL_SIZE + th_state->enq_local_index];
    TVEC_ATOMIC_ADD_BANK(&queue->enqueuers, &th_state->enq_toggle, mybank); // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier

    if (!synchIsSystemOversubscribed()) {
        volatile int k;
        int backoff_limit;

        if (synchFastRandomRange(1, queue->nthreads) > 1) {
            backoff_limit = synchFastRandomRange(th_state->max_backoff >> 1, th_state->max_backoff);
            for (k = 0; k < backoff_limit; k++)
                ;
        }
    } else if (synchFastRandomRange(1, queue->nthreads) > 4) {
        synchResched();
    }

    for (j = 0; j < 2; j++) {
        old_sp = queue->enq_sp;
        synchNonTSOFence();
        sp_data = queue->enq_pool[old_sp.struct_data.index];
        TVEC_ATOMIC_COPY_BANKS(diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(diffs, diffs, &th_state->my_enq_bit, mybank); // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                             // if the operation has already been applied return
            break;
        EnqStateCopy(lsp_data, sp_data);
        TVEC_COPY(l_toggles, &queue->enqueuers); // This is an atomic read, since sp is volatile
        if (old_sp.raw_data != queue->enq_sp.raw_data)
            continue;
        TVEC_XOR(diffs, &lsp_data->applied, l_toggles);

        EnqLinkQueue(queue, lsp_data);
        enq_counter = 1;
        node = synchAllocObj(&th_state->pool_node);
        node->next = NULL;
        node->val = arg;
        llist = node;
        TVEC_REVERSE_BIT(diffs, pid);
#ifdef DEBUG
        lsp_data->counter += 1;
#endif
        for (i = 0, prefix = 0; i < diffs->tvec_cells; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs->cell[i] != 0L) {
                register int pos, proc_id;

                pos = synchBitSearchFirst(diffs->cell[i]);
                proc_id = prefix + pos;
                enq_counter++;
#ifdef DEBUG
                lsp_data->counter += 1;
#endif
                node->next = synchAllocObj(&th_state->pool_node);
                node = (Node *)node->next;
                node->next = NULL;
                node->val = queue->announce[proc_id];
                diffs->cell[i] ^= ((bitword_t)1) << pos;
            }
        }

        lsp_data->first = lsp_data->tail;
        lsp_data->last = llist;
        lsp_data->tail = node;
        TVEC_COPY(&lsp_data->applied, l_toggles);
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;
        new_sp.struct_data.index = pid * LOCAL_POOL_SIZE + th_state->enq_local_index;
        if (old_sp.raw_data == queue->enq_sp.raw_data && synchCAS64(&queue->enq_sp, old_sp.raw_data, new_sp.raw_data)) {
            EnqLinkQueue(queue, lsp_data);
            th_state->enq_local_index = (th_state->enq_local_index + 1) % LOCAL_POOL_SIZE;
            th_state->max_backoff = (th_state->max_backoff >> 1) | 1;
            return;
        } else {
            if (th_state->max_backoff < queue->MAX_BACK)
                th_state->max_backoff <<= 1;
            synchRollback(&th_state->pool_node, enq_counter);
        }
    }

    return;
}

RetVal SimQueueDequeue(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid) {
    ToggleVector *diffs = &th_state->diffs,
                 *l_toggles = &th_state->l_toggles;
    DeqState *lsp_data, *sp_data;
    int i, j, prefix;
    pointer_t old_sp, new_sp;
    volatile Node *node;

    int mybank = TVEC_GET_BANK_OF_BIT(pid, queue->nthreads);
    TVEC_REVERSE_BIT(&th_state->my_deq_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->deq_toggle, &th_state->deq_toggle, mybank);
    lsp_data = queue->deq_pool[pid * LOCAL_POOL_SIZE + th_state->deq_local_index];
    TVEC_ATOMIC_ADD_BANK(&queue->dequeuers, &th_state->deq_toggle, mybank); // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier

    if (!synchIsSystemOversubscribed()) {
        volatile int k;
        int backoff_limit;

        if (synchFastRandomRange(1, queue->nthreads) > 1) {
            backoff_limit = synchFastRandomRange(th_state->max_backoff >> 1, th_state->max_backoff);
            for (k = 0; k < backoff_limit; k++)
                ;
        }
    } else if (synchFastRandomRange(1, queue->nthreads) > 4) {
        synchResched();
    }

    for (j = 0; j < 2; j++) {
        old_sp = queue->deq_sp;
        synchNonTSOFence();
        sp_data = queue->deq_pool[old_sp.struct_data.index];
        TVEC_ATOMIC_COPY_BANKS(diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(diffs, diffs, &th_state->my_deq_bit, mybank);        // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                                                  // if the operation has already been applied return
            break;
        DeqStateCopy(lsp_data, sp_data);
        TVEC_COPY(l_toggles, &queue->dequeuers);                                      // This is an atomic read, since sp is volatile

        if (old_sp.raw_data != queue->deq_sp.raw_data)
            continue;
        TVEC_XOR(diffs, &lsp_data->applied, l_toggles);
        DeqLinkQueue(queue, lsp_data);
        for (i = 0, prefix = 0; i < diffs->tvec_cells; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs->cell[i] != 0L) {
                register int pos, proc_id;

                pos = synchBitSearchFirst(diffs->cell[i]);
                proc_id = prefix + pos;
#ifdef DEBUG
                lsp_data->counter += 1;
#endif
                node = lsp_data->head->next;
                if (node == NULL) DeqLinkQueue(queue, lsp_data);
                node = lsp_data->head->next;
                if (node != NULL) {
                    lsp_data->ret[proc_id] = node->val;
                    lsp_data->head = (Node *)node;
                } else lsp_data->ret[proc_id] = EMPTY_QUEUE;

                diffs->cell[i] ^= ((bitword_t)1) << pos;
            }
        }
        TVEC_COPY(&lsp_data->applied, l_toggles);
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;
        new_sp.struct_data.index = pid * LOCAL_POOL_SIZE + th_state->deq_local_index;
        if (old_sp.raw_data == queue->deq_sp.raw_data && synchCAS64(&queue->deq_sp, old_sp.raw_data, new_sp.raw_data)) {
            th_state->deq_local_index = (th_state->deq_local_index + 1) % LOCAL_POOL_SIZE;
            th_state->max_backoff = (th_state->max_backoff >> 1) | 1;
            return lsp_data->ret[pid];
        } else if (th_state->max_backoff < queue->MAX_BACK)
            th_state->max_backoff <<= 1;
    }

    return queue->deq_pool[queue->deq_sp.struct_data.index]->ret[pid];
}
