#include <sim.h>

void SimInit(SimStruct *sim_struct, int max_backoff) {
    sim_struct->sp.struct_data.index = _SIM_LOCAL_POOL_SIZE_ * N_THREADS;
    sim_struct->sp.struct_data.seq = 0;
    TVEC_SET_ZERO((ToggleVector *)&sim_struct->a_toggles);

    // OBJECT'S INITIAL VALUE
    // ----------------------

    TVEC_SET_ZERO((ToggleVector *)&sim_struct->pool[_SIM_LOCAL_POOL_SIZE_ * N_THREADS].applied);
    sim_struct->MAX_BACK = max_backoff * 100;
#ifdef DEBUG
    sim_struct->pool[_SIM_LOCAL_POOL_SIZE_ * N_THREADS].counter = 0;
    sim_struct->pool[_SIM_LOCAL_POOL_SIZE_ * N_THREADS].rounds = 0;
#endif
    FullFence();
}

void SimThreadStateInit(SimThreadState *th_state, int pid) {
    TVEC_SET_ZERO(&th_state->mask);
    TVEC_SET_ZERO(&th_state->my_bit);
    TVEC_SET_ZERO(&th_state->toggle);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    th_state->toggle = TVEC_NEGATIVE(th_state->mask);
    th_state->local_index = 0;
    th_state->backoff = 1;
}

Object SimApplyOp(SimStruct *sim_struct, SimThreadState *th_state, RetVal (*sfunc)(HalfSimObjectState *, ArgVal, int), Object arg, int pid) {
    ToggleVector diffs, l_toggles;
    pointer_t old_sp, new_sp;
    HalfSimObjectState *sp_data, *lsp_data;
    int i, j, prefix, mybank;

    sim_struct->announce[pid] = arg;                                                     // sim_struct->announce the operation
    mybank = TVEC_GET_BANK_OF_BIT(pid);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->toggle, &th_state->toggle, mybank);
    lsp_data = (HalfSimObjectState *)&sim_struct->pool[pid * _SIM_LOCAL_POOL_SIZE_ + th_state->local_index];
    TVEC_ATOMIC_ADD_BANK(&sim_struct->a_toggles, &th_state->toggle, mybank);             // toggle pid's bit in sim_struct->a_toggles, Fetch&Add acts as a full write-barrier
#if N_THREADS > USE_CPUS
    if (fastRandomRange(1, N_THREADS) > 4)
        resched();
#else
    volatile int k;
    int backoff_limit;

    if (fastRandomRange(1, N_THREADS) > 1) { 
        backoff_limit = th_state->backoff;
        for (k = 0; k < backoff_limit; k++)
            ;
    }
#endif

    for (j = 0; j < 2; j++) {
        old_sp = sim_struct->sp;                                                         // read reference to struct ObjectState
        sp_data = (HalfSimObjectState *)&sim_struct->pool[old_sp.struct_data.index];     // read reference of struct ObjectState in a local variable lsim_struct->sp
        TVEC_ATOMIC_COPY_BANKS(&diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(&diffs, &diffs, &th_state->my_bit, mybank);                       // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                                                     // if the operation has already been applied return
            break;
        *lsp_data = *sp_data;
        l_toggles = sim_struct->a_toggles;                                               // This is an atomic read, since a_toogles is volatile
        if (old_sp.raw_data != sim_struct->sp.raw_data)
            continue;
        diffs = TVEC_XOR(lsp_data->applied, l_toggles);
#ifdef DEBUG
        lsp_data->rounds++;
#endif
        lsp_data->ret[pid]=sfunc(lsp_data, arg, pid);
        TVEC_REVERSE_BIT(&diffs, pid);
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            ReadPrefetch(&sim_struct->announce[prefix]);
#if N_THREADS > 7
            ReadPrefetch(&sim_struct->announce[prefix + 8]);
#endif
#if N_THREADS > 15
            ReadPrefetch(&sim_struct->announce[prefix + 16]);
#endif
#if N_THREADS > 23
            ReadPrefetch(&sim_struct->announce[prefix + 24]);
#endif
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                diffs.cell[i] ^= ((bitword_t)1) << pos;
                lsp_data->ret[proc_id] = sfunc(lsp_data, sim_struct->announce[proc_id], proc_id);
            }
        }
        lsp_data->applied = l_toggles;                                                   // change applied to be equal to what was read in sim_struct->a_toggles
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;                             // increase timestamp
        new_sp.struct_data.index = _SIM_LOCAL_POOL_SIZE_ * pid + th_state->local_index;  // store in mod_dw.index the index in sim_struct->pool where lsim_struct->sp will be stored
        if (old_sp.raw_data==sim_struct->sp.raw_data && 
            CAS64(&sim_struct->sp, old_sp.raw_data, new_sp.raw_data)) {                  // try to change sim_struct->sp to the value mod_dw
            th_state->local_index = (th_state->local_index + 1) % _SIM_LOCAL_POOL_SIZE_; // if this happens successfully,use next item in pid's sim_struct->pool next time
            th_state->backoff = (th_state->backoff >> 1) | 1;
            return lsp_data->ret[pid];
        } else if (th_state->backoff < sim_struct->MAX_BACK) th_state->backoff <<= 1;
    }
    return sim_struct->pool[sim_struct->sp.struct_data.index].ret[pid];                  // return the value found in the record stored there
}
