#include <lfuobject.h>

void LFUObjectInit(LFUObjectStruct *l, ArgVal value) {
    l->state.state = value;
    FullFence();
}

void LFUObjectThreadStateInit(LFUObjectThreadState *th_state, int min_back, int max_back) {
    synchInitBackoff(&th_state->backoff, min_back, max_back, 1);
}

RetVal LFUObjectApplyOp(LFUObjectStruct *l, LFUObjectThreadState *th_state, RetVal (*sfunc)(void *, ArgVal, int), ArgVal arg, int pid) {
    ObjectState new_state, old_state, ret_state;

    synchResetBackoff(&th_state->backoff);
    do {
        old_state.state = l->state.state;
        new_state.state = old_state.state;
        ret_state.state = sfunc(&new_state.state, arg, pid);

        if (CAS64(&l->state.state, old_state.state, new_state.state) == true) {
            break;
        } else
            synchBackoffDelay(&th_state->backoff);
    } while (true);

    return ret_state.state;
}
