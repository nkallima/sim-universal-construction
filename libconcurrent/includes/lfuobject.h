#ifndef _LFUOBJECT_H_
#define _LFUOBJECT_H_

#include <config.h>
#include <primitives.h>
#include <backoff.h>
#include <fastrand.h>
#include <pool.h>
#include <threadtools.h>
#include <fam.h>

typedef union LFUObject {
    volatile ObjectState state;
    char pad[CACHE_LINE_SIZE];
} LFUObject;

typedef struct LFUObjectThreadState {
    BackoffStruct backoff;
} LFUObjectThreadState;

void LFUObjectInit(LFUObject *l, ArgVal value);
void LFUObjectThreadStateInit(LFUObjectThreadState *th_state, int min_back, int max_back);
RetVal LFUObjectApplyOp(LFUObject *l, LFUObjectThreadState *th_state, RetVal (*sfunc)(void *, ArgVal, int), ArgVal arg, int pid);

#endif
