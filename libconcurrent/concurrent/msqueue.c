#include <msqueue.h>

void MSQueueInit(MSQueue *l) {
    Node *p = getMemory(sizeof(Node));

    p->next = null;
    l->head = p;
    l->tail = p;
    FullFence();
}

void MSQueueThreadStateInit(MSQueueThreadState *th_state, int min_back, int max_back) {
    init_backoff(&th_state->backoff, min_back, max_back, 1);
    init_pool(&th_state->pool, sizeof(Node));
}

void MSQueueEnqueue(MSQueue *l, MSQueueThreadState *th_state, ArgVal arg) {
    Node *p;
    Node *next, *last;

    p = alloc_obj(&th_state->pool);
    p->val = arg;
    p->next = null;
    reset_backoff(&th_state->backoff);
    while (true) {
        last = (Node *)l->tail;
        next = (Node *)last->next;
        if (last == l->tail) {
            if (next == null) {
                reset_backoff(&th_state->backoff);
                if (CASPTR(&last->next, next, p))
                    break;
            } else {
                CASPTR(&l->tail, last, next);
                backoff_delay(&th_state->backoff);
            }
        }
    }
    CASPTR(&l->tail, last, p);
}

RetVal MSQueueDequeue(MSQueue *l, MSQueueThreadState *th_state) {
    Node *first, *last, *next;
    Object value;

    reset_backoff(&th_state->backoff);
    while (true) {
        first = (Node *)l->head;
        last = (Node *)l->tail;
        next = (Node *)first->next;
        if (first == l->head) {
            if (first == last) {
                if (next == null)
                    return EMPTY_QUEUE;
                CASPTR(&l->tail, last, next);
                backoff_delay(&th_state->backoff);
            } else {
                value = next->val;
                if (CASPTR(&l->head, first, next))
                    break;
                backoff_delay(&th_state->backoff);
            }
        }
    }
    return value;
}
