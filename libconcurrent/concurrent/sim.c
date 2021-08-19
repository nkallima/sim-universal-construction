#include <sim.h>
#include <fastrand.h>
#include <threadtools.h>

static inline void SimStateCopy(SimObjectState *dest, SimObjectState *src);

static inline void SimStateCopy(SimObjectState *dest, SimObjectState *src) {
    // copy everything except 'applied' and 'ret' fields
    memcpy(&dest->state, &src->state, SimObjectStateSize(dest->applied.nthreads) - CACHE_LINE_SIZE);
}

void synchSimStructInit(SimStruct *sim_struct, uint32_t nthreads, int max_backoff) {
    int i;

    sim_struct->nthreads = nthreads;
    TVEC_INIT_AT((ToggleVector *)&sim_struct->a_toggles, nthreads, synchGetAlignedMemory(CACHE_LINE_SIZE, _TVEC_VECTOR_SIZE(nthreads)));
    sim_struct->announce = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(ArgVal));
    sim_struct->pool = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(SimObjectState *) * (_SIM_LOCAL_POOL_SIZE_ * nthreads + 1));
    for (i = 0; i < _SIM_LOCAL_POOL_SIZE_ * nthreads + 1; i++) {
        sim_struct->pool[i] = synchGetAlignedMemory(CACHE_LINE_SIZE, SimObjectStateSize(nthreads));
        TVEC_INIT_AT(&sim_struct->pool[i]->applied, nthreads, sim_struct->pool[i]->__flex);
        sim_struct->pool[i]->ret = ((void *)sim_struct->pool[i]->__flex) + _TVEC_VECTOR_SIZE(nthreads);
    }

    sim_struct->sp.struct_data.index = _SIM_LOCAL_POOL_SIZE_ * nthreads;
    sim_struct->sp.struct_data.seq = 0;

    // OBJECT'S INITIAL VALUE
    // ----------------------
    sim_struct->pool[_SIM_LOCAL_POOL_SIZE_ * nthreads]->state.state = 0;
    TVEC_SET_ZERO((ToggleVector *)&sim_struct->pool[_SIM_LOCAL_POOL_SIZE_ * nthreads]->applied);
    sim_struct->MAX_BACK = max_backoff * 100;
#ifdef DEBUG
    sim_struct->pool[_SIM_LOCAL_POOL_SIZE_ * nthreads]->counter = 0;
    sim_struct->pool[_SIM_LOCAL_POOL_SIZE_ * nthreads]->rounds = 0;
#endif
    FullFence();
}

void SimThreadStateInit(SimThreadState *th_state, uint32_t nthreads, int pid) {
    TVEC_INIT(&th_state->mask, nthreads);
    TVEC_INIT(&th_state->toggle, nthreads);
    TVEC_INIT(&th_state->my_bit, nthreads);
    TVEC_INIT(&th_state->diffs, nthreads);
    TVEC_INIT(&th_state->l_toggles, nthreads);

    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    TVEC_NEGATIVE(&th_state->toggle, &th_state->mask);
    th_state->local_index = 0;
    th_state->backoff = 1;
}

Object SimApplyOp(SimStruct *sim_struct, SimThreadState *th_state, RetVal (*sfunc)(void *, ArgVal, int), Object arg, int pid) {
    ToggleVector *diffs = &th_state->diffs, *l_toggles = &th_state->l_toggles;
    pointer_t old_sp, new_sp;
    HalfSimObjectState *sp_data, *lsp_data;
    int i, j, prefix, mybank;

    sim_struct->announce[pid] = arg;                                                    // sim_struct->announce the operation
    mybank = TVEC_GET_BANK_OF_BIT(pid, sim_struct->nthreads);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->toggle, &th_state->toggle, mybank);
    lsp_data = (HalfSimObjectState *)sim_struct->pool[pid * _SIM_LOCAL_POOL_SIZE_ + th_state->local_index];
    TVEC_ATOMIC_ADD_BANK(&sim_struct->a_toggles, &th_state->toggle, mybank);            // toggle pid's bit in sim_struct->a_toggles, Fetch&Add acts as a full write-barrier

    if (!synchIsSystemOversubscribed()) {
        volatile int k;
        int backoff_limit;

        if (synchFastRandomRange(1, sim_struct->nthreads) > 1) {
            backoff_limit = th_state->backoff;
            for (k = 0; k < backoff_limit; k++)
                ;
        }
    } else if (synchFastRandomRange(1, sim_struct->nthreads) > 4)
        synchResched();

    for (j = 0; j < 2; j++) {
        old_sp = sim_struct->sp;                                                        // read reference to struct ObjectState
        sp_data = (HalfSimObjectState *)sim_struct->pool[old_sp.struct_data.index];     // read reference of struct ObjectState in a local variable lsim_struct->sp
        TVEC_ATOMIC_COPY_BANKS(diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(diffs, diffs, &th_state->my_bit, mybank);                        // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                                                    // if the operation has already been applied return
            break;
        SimStateCopy((SimObjectState *)lsp_data, (SimObjectState *)sp_data);
        TVEC_COPY(l_toggles, (ToggleVector *)&sim_struct->a_toggles);                   // This is an atomic read, since a_toogles is volatile
        if (old_sp.raw_data != sim_struct->sp.raw_data)
            continue;
        TVEC_XOR(diffs, &lsp_data->applied, l_toggles);
#ifdef DEBUG
        lsp_data->rounds++;
        lsp_data->counter++;
#endif
        lsp_data->ret[pid] = sfunc(&lsp_data->state, arg, pid);
        TVEC_REVERSE_BIT(diffs, pid);
        for (i = 0, prefix = 0; i < diffs->tvec_cells; i++, prefix += _TVEC_BIWORD_SIZE_) {
            ReadPrefetch(&sim_struct->announce[prefix]);
            ReadPrefetch(&sim_struct->announce[prefix + 8]);
            ReadPrefetch(&sim_struct->announce[prefix + 16]);
            ReadPrefetch(&sim_struct->announce[prefix + 24]);

            while (diffs->cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs->cell[i]);
                proc_id = prefix + pos;
                diffs->cell[i] ^= ((bitword_t)1) << pos;
                lsp_data->ret[proc_id] = sfunc(&lsp_data->state, sim_struct->announce[proc_id], proc_id);
#ifdef DEBUG
                lsp_data->counter++;
#endif
            }
        }
        TVEC_COPY(&lsp_data->applied, l_toggles);                                       // change applied to be equal to what was read in sim_struct->a_toggles
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;                            // increase timestamp
        new_sp.struct_data.index = _SIM_LOCAL_POOL_SIZE_ * pid + th_state->local_index; // store in mod_dw.index the index in sim_struct->pool where lsim_struct->sp will be stored
        if (old_sp.raw_data == sim_struct->sp.raw_data && CAS64(&sim_struct->sp, old_sp.raw_data, new_sp.raw_data)) { // try to change sim_struct->sp to the value mod_dw
            th_state->local_index = (th_state->local_index + 1) % _SIM_LOCAL_POOL_SIZE_;                              // if this happens successfully,use next item in pid's sim_struct->pool next time
            th_state->backoff = (th_state->backoff >> 1) | 1;
            return lsp_data->ret[pid];
        } else if (th_state->backoff < sim_struct->MAX_BACK)
            th_state->backoff <<= 1;
    }
    return sim_struct->pool[sim_struct->sp.struct_data.index]->ret[pid];                // return the value found in the record stored there
}
