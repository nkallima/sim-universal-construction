#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "threadtools.h"
#include "fc.h"

#define FC_CLEANUP_FREQUENCY     100
#define FC_CLEANUP_OLD_THRESHOLD 100
#define FC_COMBINING_ROUNDS      3

static void FCEnqueueRequest(FCStruct *lock, FCThreadState *st_thread);

void FCStructInit(FCStruct *l, uint32_t nthreads) {
    l->lock = 0;
    l->count = 0;
    l->head = NULL;
    l->counter = 0;
    l->rounds = 0;
    l->nodes = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(FCRequest));
    synchStoreFence();
}

void FCThreadStateInit(FCStruct *l, FCThreadState *st_thread, int pid) {
    st_thread->node = &l->nodes[pid];
    st_thread->node->age = 0;
    st_thread->node->active = false;
}

static void FCEnqueueRequest(FCStruct *lock, FCThreadState *st_thread) {
    FCRequest *request = st_thread->node;
    FCRequest *supposed;
    request->active = true;

    do {
        synchResched();
        supposed = (FCRequest *)lock->head;
        request->next = supposed;
    } while (!synchCASPTR(&lock->head, supposed, request));
}

RetVal FCApplyOp(FCStruct *lock, FCThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    struct FCRequest *request;
    int i;

    request = st_thread->node;
    request->val = arg;
    request->pending = true;
    synchStoreFence();

    while (true) {
        if (lock->lock == 0 && synchCAS32(&lock->lock, 0, 1)) {
            break;
        } else {
            while (lock->lock && request->pending && request->active) {
                synchResched();
            }
            if (request->pending == false) {
                return request->val;
            } else if (request->active == false) {
                FCEnqueueRequest(lock, st_thread);
            }
        }
    }
    if (request->active == false) FCEnqueueRequest(lock, st_thread);
    lock->count = 1;
    int count = lock->count;
    volatile FCRequest *cur;

#ifdef DEBUG
    lock->rounds += 1;
#endif
    for (i = 0; i < FC_COMBINING_ROUNDS; i++) {
        for (cur = lock->head; cur != NULL; cur = cur->next) {
            if (cur->pending) {
                cur->val = sfunc(state, arg, pid);
                cur->pending = false;
                cur->age = count;
#ifdef DEBUG
                lock->counter += 1;
#endif
            }
        }
    }

    if (!(count % FC_CLEANUP_FREQUENCY)) {
        volatile FCRequest *prev = (FCRequest *)lock->head;
        while ((cur = prev->next)) {
            if ((cur->age + FC_CLEANUP_OLD_THRESHOLD) < count) {
                prev->next = cur->next;
                cur->active = 0;
            } else
                prev = cur;
        }
    }
    lock->lock = 0;
    synchStoreFence();

    return request->val;
}
