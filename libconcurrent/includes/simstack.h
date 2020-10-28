#ifndef _SIMSTACK_H_
#define _SIMSTACK_H_

#include <sim.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>
#include <uthreads.h>

typedef struct HalfObjectState {
    ToggleVector applied;
    Object *ret;
    Node *head;
#ifdef DEBUG
    int counter;
#endif
    uint64_t __flex[1];
} HalfObjectState;

typedef struct ObjectState {
    ToggleVector applied;
    Object *ret;
    Node *head;
#ifdef DEBUG
    int counter;
#endif
    uint64_t __flex[1];
    char pad[PAD_CACHE(sizeof(HalfObjectState))];
} ObjectState;

#define ObjectStateSize(nthreads) (sizeof(ObjectState) + _TVEC_VECTOR_SIZE(nthreads) + nthreads * sizeof(Object))

typedef struct SimStackThreadState {
    PoolStruct pool;
    ToggleVector mask;
    ToggleVector toggle;
    ToggleVector my_bit;
    ToggleVector diffs;
    ToggleVector l_toggles;
    ToggleVector pops;
    int local_index;
    int backoff;
} SimStackThreadState;

typedef struct SimStackStruct {
    ArgVal *announce;
    ObjectState **pool;
    uint32_t nthreads;
    int MAX_BACK;
    volatile Node *head CACHE_ALIGN;
    volatile pointer_t sp CACHE_ALIGN;
    volatile ToggleVector a_toggles CACHE_ALIGN;
} SimStackStruct;

void SimStackInit(SimStackStruct *stack, uint32_t nthreads, int max_backoff);
void SimStackThreadStateInit(SimStackThreadState *th_state, uint32_t nthreads, int pid);
void SimStackPush(SimStackStruct *stack, SimStackThreadState *th_state, ArgVal arg, int pid);
RetVal SimStackPop(SimStackStruct *stack, SimStackThreadState *th_state, int pid);

#endif
