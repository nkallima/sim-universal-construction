#ifndef _MSQUEUE_H_
#define _MSQUEUE_H_

#include <config.h>
#include <primitives.h>
#include <backoff.h>
#include <pool.h>
#include <queue-stack.h>

typedef struct MSQueue {
    volatile Node *head CACHE_ALIGN;
    volatile Node *tail CACHE_ALIGN;
} MSQueue;

typedef struct MSQueueThreadState {
    PoolStruct pool;
    BackoffStruct backoff;
} MSQueueThreadState;

void MSQueueInit(MSQueue *l);
void MSQueueThreadStateInit(MSQueueThreadState *th_state, int min_back, int max_back);
void MSQueueEnqueue(MSQueue *l, MSQueueThreadState *th_state, ArgVal arg);
RetVal MSQueueDequeue(MSQueue *l, MSQueueThreadState *th_state);

#endif