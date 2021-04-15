#ifndef _LFSTACK_H_
#define _LFSTACK_H_

#include <config.h>
#include <queue-stack.h>
#include <primitives.h>
#include <backoff.h>
#include <pool.h>

typedef struct LFStack {
    volatile Node *top;
} LFStack;

typedef struct LFStackThreadState {
    PoolStruct pool;
    BackoffStruct backoff;
} LFStackThreadState;

void LFStackInit(LFStack *l);
void LFStackThreadStateInit(LFStackThreadState *th_state, int min_back, int max_back);
void LFStackPush(LFStack *l, LFStackThreadState *th_state, ArgVal arg);
RetVal LFStackPop(LFStack *l, LFStackThreadState *th_state);

#endif