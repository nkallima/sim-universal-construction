#include <lfstack.h>

inline void LFStackInit(LFStackStruct *l) {
    l->top = NULL;
    FullFence();
}

inline void LFStackThreadStateInit(LFStackThreadState *th_state, int min_back, int max_back) {
    init_backoff(&th_state->backoff, min_back, max_back, 1);
    init_pool(&th_state->pool, sizeof(Node));
}

inline void LFStackPush(LFStackStruct *l, LFStackThreadState *th_state, ArgVal arg) {
    Node *n;

    n = alloc_obj(&th_state->pool);
    reset_backoff(&th_state->backoff);
    n->val = arg;
    do {
        Node *old_top = (Node *)l->top; // top is volatile
        n->next = old_top;
        if (CASPTR(&l->top, old_top, n) == true)
            break;
        else
            backoff_delay(&th_state->backoff);
    } while (true);
}

inline RetVal LFStackPop(LFStackStruct *l, LFStackThreadState *th_state) {
    reset_backoff(&th_state->backoff);
    do {
        Node *old_top = (Node *)l->top;
        if (old_top == NULL)
            return EMPTY_STACK;
        if (CASPTR(&l->top, old_top, old_top->next))
            return old_top->val;
        else
            backoff_delay(&th_state->backoff);
    } while (true);
}
