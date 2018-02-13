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
    Node *head;
    Object ret[N_THREADS];
    #ifdef DEBUG
    int counter;
    #endif
} HalfObjectState;


typedef struct ObjectState {
    ToggleVector applied;
    Node *head;
    Object ret[N_THREADS];
    #ifdef DEBUG
    int counter;
    #endif
    int32_t pad[PAD_CACHE(sizeof(HalfObjectState))];
} ObjectState;

typedef struct SimStackThreadState {
    PoolStruct pool;
    ToggleVector mask CACHE_ALIGN;
    ToggleVector toggle;
    ToggleVector my_bit;
    int local_index;
    int backoff;
} SimStackThreadState;


typedef struct SimStackStruct {
    volatile Node *head CACHE_ALIGN;
    volatile pointer_t sp CACHE_ALIGN;
    volatile ToggleVector a_toggles CACHE_ALIGN;
    volatile ArgVal announce[N_THREADS] CACHE_ALIGN;
    volatile ObjectState pool[N_THREADS * _SIM_LOCAL_POOL_SIZE_ + 1] CACHE_ALIGN;
    int MAX_BACK CACHE_ALIGN;
} SimStackStruct;

void SimStackThreadStateInit(SimStackThreadState *th_state, int pid);
void SimStackInit(SimStackStruct *stack, int max_backoff);
void SimStackPush(SimStackStruct *stack, SimStackThreadState *th_state, ArgVal arg, int pid);
RetVal SimStackPop(SimStackStruct *stack, SimStackThreadState *th_state, int pid);

#endif