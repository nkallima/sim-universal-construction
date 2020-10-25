#include <lfuobject.h>

void LFUObjectInit(LFUObject *l, ArgVal value) {
    l->state.state = value;
    FullFence();
}

void LFUObjectThreadStateInit(LFUObjectThreadState *th_state, int min_back, int max_back) {
    init_backoff(&th_state->backoff, min_back, max_back, 1);
}

RetVal LFUObjectApplyOp(LFUObject *l, LFUObjectThreadState *th_state, RetVal (*sfunc)(void *, ArgVal, int), ArgVal arg, int pid) {
    ObjectState new_state, old_state, ret_state;

    reset_backoff(&th_state->backoff);
    do {
        old_state.state = l->state.state;
        new_state.state = old_state.state;
        ret_state.state = sfunc(&new_state.state, arg, pid);

        if (CAS64(&l->state.state, old_state.state, new_state.state) == true) {
            break;
        }
        else backoff_delay(&th_state->backoff);
    } while(true);

    return old_state.state;
}
