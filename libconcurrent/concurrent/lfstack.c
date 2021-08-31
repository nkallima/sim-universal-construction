#include <lfstack.h>

inline void LFStackInit(LFStackStruct *l) {
    l->top = NULL;
    synchFullFence();
}

inline void LFStackThreadStateInit(LFStackThreadState *th_state, int min_back, int max_back) {
    synchInitBackoff(&th_state->backoff, min_back, max_back, 1);
    synchInitPool(&th_state->pool, sizeof(Node));
}

inline void LFStackPush(LFStackStruct *l, LFStackThreadState *th_state, ArgVal arg) {
    Node *n;

    n = synchAllocObj(&th_state->pool);
    synchResetBackoff(&th_state->backoff);
    n->val = arg;
    do {
        Node *old_top = (Node *)l->top; // top is volatile
        n->next = old_top;
        if (synchCASPTR(&l->top, old_top, n) == true)
            break;
        else
            synchBackoffDelay(&th_state->backoff);
    } while (true);
}

inline RetVal LFStackPop(LFStackStruct *l, LFStackThreadState *th_state) {
    synchResetBackoff(&th_state->backoff);
    do {
        Node *old_top = (Node *)l->top;
        if (old_top == NULL)
            return EMPTY_STACK;
        if (synchCASPTR(&l->top, old_top, old_top->next))
            return old_top->val;
        else
            synchBackoffDelay(&th_state->backoff);
    } while (true);
}
