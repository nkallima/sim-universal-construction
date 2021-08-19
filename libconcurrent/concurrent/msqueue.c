#include <msqueue.h>

void MSQueueInit(MSQueueStruct *l) {
    Node *p = synchGetMemory(sizeof(Node));

    p->next = NULL;
    l->head = p;
    l->tail = p;
    FullFence();
}

void MSQueueThreadStateInit(MSQueueThreadState *th_state, int min_back, int max_back) {
    synchInitBackoff(&th_state->backoff, min_back, max_back, 1);
    synchInitPool(&th_state->pool, sizeof(Node));
}

void MSQueueEnqueue(MSQueueStruct *l, MSQueueThreadState *th_state, ArgVal arg) {
    Node *p;
    Node *next, *last;

    p = synchAllocObj(&th_state->pool);
    p->val = arg;
    p->next = NULL;
    synchResetBackoff(&th_state->backoff);
    while (true) {
        last = (Node *)l->tail;
        next = (Node *)last->next;
        if (last == l->tail) {
            if (next == NULL) {
                synchResetBackoff(&th_state->backoff);
                if (CASPTR(&last->next, next, p))
                    break;
            } else {
                CASPTR(&l->tail, last, next);
                synchBackoffDelay(&th_state->backoff);
            }
        }
    }
    CASPTR(&l->tail, last, p);
}

RetVal MSQueueDequeue(MSQueueStruct *l, MSQueueThreadState *th_state) {
    Node *first, *last, *next;
    Object value;

    synchResetBackoff(&th_state->backoff);
    while (true) {
        first = (Node *)l->head;
        last = (Node *)l->tail;
        next = (Node *)first->next;
        if (first == l->head) {
            if (first == last) {
                if (next == NULL)
                    return EMPTY_QUEUE;
                CASPTR(&l->tail, last, next);
                synchBackoffDelay(&th_state->backoff);
            } else {
                value = next->val;
                if (CASPTR(&l->head, first, next))
                    break;
                synchBackoffDelay(&th_state->backoff);
            }
        }
    }
    return value;
}
