#include <lfuobject.h>

void LFUObjectInit(LFUObject *l, ArgVal value) {
    l->val = value;
    FullFence();
}

void LFUObjectThreadStateInit(LFUObjectThreadState *th_state, int min_back, int max_back) {
    init_backoff(&th_state->backoff, min_back, max_back, 1);
}

RetVal LFUObjectApplyOp(LFUObject *l, LFUObjectThreadState *th_state, RetVal (*sfunc)(Object, ArgVal, int), ArgVal arg, int pid) {
    RetVal old_val = arg, new_val;
    reset_backoff(&th_state->backoff);
    do {
        old_val = l->val;   // val is volatile
		new_val = sfunc(old_val, arg, pid);
        if (CAS64(&l->val, old_val, new_val) == true)
            break;
        else backoff_delay(&th_state->backoff);
    } while(true);

    return old_val;
}
