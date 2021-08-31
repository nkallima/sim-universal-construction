#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <tvec.h>
#include <pool.h>
#include <threadtools.h>
#include <simstack.h>

static const uint64_t POP = LLONG_MIN;

inline static void serialPush(HalfSimStackState *st, SimStackThreadState *th_state, ArgVal arg);
inline static bool serialPop(HalfSimStackState *st, int pid);
inline static RetVal SimStackApplyOp(SimStackStruct *stack, SimStackThreadState *th_state, ArgVal arg, int pid);
static inline void SimStackStateCopy(SimStackState *dest, SimStackState *src);
inline static void recycleList(SynchPoolStruct *pool, Node *head, uint32_t items);

static inline void SimStackStateCopy(SimStackState *dest, SimStackState *src) {
    // copy everything except 'applied' and 'ret' fields
    memcpy(&dest->head, &src->head, SimStackStateSize(dest->applied.nthreads) - sizeof(ToggleVector) - sizeof(Object *));
}

void SimStackStructInit(SimStackStruct *stack, uint32_t nthreads, int max_backoff) {
    int i;

    stack->nthreads = nthreads;
    stack->a_toggles.cell = synchGetAlignedMemory(CACHE_LINE_SIZE, _TVEC_VECTOR_SIZE(nthreads));
    stack->announce = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(ArgVal));
    stack->pool = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(SimStackState *) * (_SIM_LOCAL_POOL_SIZE_ * nthreads + 1));
    for (i = 0; i < _SIM_LOCAL_POOL_SIZE_ * nthreads + 1; i++) {
        stack->pool[i] = synchGetAlignedMemory(CACHE_LINE_SIZE, SimStackStateSize(nthreads));
        TVEC_INIT_AT(&stack->pool[i]->applied, nthreads, stack->pool[i]->__flex);
        stack->pool[i]->ret = ((void *)stack->pool[i]->__flex) + _TVEC_VECTOR_SIZE(nthreads);
    }
    stack->sp.struct_data.index = _SIM_LOCAL_POOL_SIZE_ * nthreads;
    stack->sp.struct_data.seq = 0;
    TVEC_INIT((ToggleVector *)&stack->a_toggles, nthreads);
    stack->pool[_SIM_LOCAL_POOL_SIZE_ * nthreads]->head = NULL;

    TVEC_SET_ZERO((ToggleVector *)&stack->pool[_SIM_LOCAL_POOL_SIZE_ * nthreads]->applied);
    stack->MAX_BACK = max_backoff * 100;
#ifdef DEBUG
    stack->pool[_SIM_LOCAL_POOL_SIZE_ * nthreads]->counter = 0;
#endif
    synchFullFence();
}

void SimStackThreadStateInit(SimStackThreadState *th_state, uint32_t nthreads, int pid) {
    TVEC_INIT(&th_state->diffs, nthreads);
    TVEC_INIT(&th_state->l_toggles, nthreads);
    TVEC_INIT(&th_state->mask, nthreads);
    TVEC_INIT(&th_state->my_bit, nthreads);
    TVEC_INIT(&th_state->toggle, nthreads);
    TVEC_INIT(&th_state->pops, nthreads);

    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    TVEC_NEGATIVE(&th_state->toggle, &th_state->mask);
    th_state->local_index = 0;
    th_state->backoff = 1;
    synchInitPool(&th_state->pool, sizeof(Node));
}

inline static void recycleList(SynchPoolStruct *pool, Node *head, uint32_t items) {
    while (items > 0) {
        Node *node = head;
        head = (Node *)head->next;
        items--;
        synchRecycleObj(pool, node);
    }
}

inline static void serialPush(HalfSimStackState *st, SimStackThreadState *th_state, ArgVal arg) {
#ifdef DEBUG
    st->counter += 1;
#endif
    Node *n;
    n = synchAllocObj(&th_state->pool);
    n->val = (ArgVal)arg;
    n->next = st->head;
    st->head = n;
}

inline static bool serialPop(HalfSimStackState *st, int pid) {
#ifdef DEBUG
    st->counter += 1;
#endif
    if (st->head != NULL) {
        st->ret[pid] = (RetVal)st->head->val;
        st->head = (Node *)st->head->next;
        return true;
    } else {
        st->ret[pid] = EMPTY_STACK;
        return false;
    }
}

inline static RetVal SimStackApplyOp(SimStackStruct *stack, SimStackThreadState *th_state, ArgVal arg, int pid) {
    ToggleVector *diffs = &th_state->diffs, *l_toggles = &th_state->l_toggles, *pops = &th_state->pops;
    pointer_t new_sp, old_sp;
    HalfSimStackState *lsp_data, *sp_data;
    int i, j, prefix, mybank;

    mybank = TVEC_GET_BANK_OF_BIT(pid, stack->nthreads);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->toggle, &th_state->toggle, mybank);
    lsp_data = (HalfSimStackState *)stack->pool[pid * _SIM_LOCAL_POOL_SIZE_ + th_state->local_index];
    stack->announce[pid] = arg;                                                         // stack->announce the operation
    TVEC_ATOMIC_ADD_BANK(&stack->a_toggles, &th_state->toggle, mybank);                 // toggle pid's bit in stack->a_toggles, Fetch&Add acts as a full write-barrier

    if (!synchIsSystemOversubscribed()) {
        volatile int k;
        int backoff_limit;

        if (synchFastRandomRange(1, stack->nthreads) > 1) {
            backoff_limit = synchFastRandomRange(th_state->backoff >> 1, th_state->backoff);
            for (k = 0; k < backoff_limit; k++)
                ;
        }
    } else
        synchResched();

    for (j = 0; j < 2; j++) {
        old_sp = stack->sp;                                                             // read reference to struct SimStackState
        sp_data = (HalfSimStackState *)stack->pool[old_sp.struct_data.index];           // read reference of struct SimStackState in a local variable lsp_data
        TVEC_ATOMIC_COPY_BANKS(diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(diffs, diffs, &th_state->my_bit, mybank);                        // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                                                    // if the operation has already been applied return
            break;
        SimStackStateCopy((SimStackState *)lsp_data, (SimStackState *)sp_data);
        TVEC_COPY(l_toggles, (ToggleVector *)&stack->a_toggles);                        // This is an atomic read, since a_toogles is volatile
        if (old_sp.raw_data != stack->sp.raw_data)
            continue;
        TVEC_XOR(diffs, &lsp_data->applied, l_toggles);

        TVEC_SET_ZERO(pops);
        int push_counter = 0;
        for (i = 0, prefix = 0; i < diffs->tvec_cells; i++, prefix += _TVEC_BIWORD_SIZE_) {
            synchReadPrefetch(&stack->announce[prefix]);
            synchReadPrefetch(&stack->announce[prefix + 8]);
            synchReadPrefetch(&stack->announce[prefix + 16]);
            synchReadPrefetch(&stack->announce[prefix + 24]);

            while (diffs->cell[i] != 0L) {
                register int pos, proc_id;

                pos = synchBitSearchFirst(diffs->cell[i]);
                proc_id = prefix + pos;
                diffs->cell[i] ^= ((bitword_t)1) << pos;
                if (stack->announce[proc_id] == POP) {
                    pops->cell[i] |= ((bitword_t)1) << pos;
                } else {
                    serialPush(lsp_data, th_state, stack->announce[proc_id]);
                    push_counter++;
                }
            }
        }

        Node *free_list = lsp_data->head;
        int pop_counter = 0;
        for (i = 0, prefix = 0; i < pops->tvec_cells; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (pops->cell[i] != 0L) {
                register int pos, proc_id;

                pos = synchBitSearchFirst(pops->cell[i]);
                proc_id = prefix + pos;
                pops->cell[i] ^= ((bitword_t)1) << pos;
                pop_counter += serialPop(lsp_data, proc_id);
                synchNonTSOFence();
                if (old_sp.raw_data != stack->sp.raw_data)
                    goto outer;
            }
        }
 outer:
        TVEC_COPY(&lsp_data->applied, l_toggles);                                       // change applied to be equal to what was read in stack->a_toggles
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;                            // increase timestamp
        new_sp.struct_data.index = _SIM_LOCAL_POOL_SIZE_ * pid + th_state->local_index; // store in mod_dw.index the index in stack->pool where lsp_data will be stored
        if (old_sp.raw_data == stack->sp.raw_data && synchCAS64(&stack->sp.raw_data, old_sp.raw_data, new_sp.raw_data)) {
            th_state->local_index = (th_state->local_index + 1) % _SIM_LOCAL_POOL_SIZE_;
            th_state->backoff = (th_state->backoff >> 1) | 1;
            recycleList(&th_state->pool, free_list, pop_counter);

            return lsp_data->ret[pid];
        } else {
            if (th_state->backoff < stack->MAX_BACK)
                th_state->backoff <<= 1;
            recycleList(&th_state->pool, free_list, push_counter);
        }
    }

    return stack->pool[stack->sp.struct_data.index]->ret[pid];                          // return the value found in the record stored there
}

void SimStackPush(SimStackStruct *stack, SimStackThreadState *th_state, ArgVal arg, int pid) {
    SimStackApplyOp(stack, th_state, arg, pid);
}

RetVal SimStackPop(SimStackStruct *stack, SimStackThreadState *th_state, int pid) {
    return SimStackApplyOp(stack, th_state, POP, pid);
}