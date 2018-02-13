#ifndef _SIMQUEUE_H_
#define _SIMQUEUE_H_

#include <config.h>
#include <primitives.h>
#include <tvec.h>
#include <fastrand.h>
#include <pool.h>
#include <threadtools.h>
#include <sim.h>
#include <queue-stack.h>

typedef struct HalfEnqState {
    ToggleVector applied;
    Node *link_a;
    Node *link_b;
    Node *ptr;
#ifdef DEBUG
    int32_t counter;
#endif
} HalfEnqState;

typedef struct EnqState {
    ToggleVector applied;
    Node *link_a;
    Node *link_b;
    Node *ptr;
#ifdef DEBUG
    int32_t counter;
#endif
    int32_t pad[PAD_CACHE(sizeof (HalfEnqState))];
} EnqState;

typedef struct HalfDeqState {
    ToggleVector applied;
    Node *ptr;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int32_t counter;
#endif
} HalfDeqState;


typedef struct DeqState {
    ToggleVector applied;
    Node *ptr;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int32_t counter;
#endif
    int32_t pad[PAD_CACHE(sizeof (HalfDeqState))];
} DeqState;

// Each thread owns a private copy of the following variables
typedef struct SimQueueThreadState {
    ToggleVector mask;
    ToggleVector deq_toggle;
    ToggleVector my_deq_bit;
    ToggleVector enq_toggle;
    ToggleVector my_enq_bit;
    PoolStruct pool_node;
    int deq_local_index;
    int enq_local_index;
    int mybank;
    int backoff;
} SimQueueThreadState;

typedef struct SimQueueStruct {
    volatile pointer_t enq_sp CACHE_ALIGN;
    volatile pointer_t deq_sp CACHE_ALIGN;
    volatile ToggleVector enqueuers CACHE_ALIGN;
    volatile ToggleVector dequeuers CACHE_ALIGN;
    volatile ArgVal announce[N_THREADS] CACHE_ALIGN;
    EnqState enq_pool[_SIM_LOCAL_POOL_SIZE_ * N_THREADS + 1] CACHE_ALIGN;
    DeqState deq_pool[_SIM_LOCAL_POOL_SIZE_ * N_THREADS + 1] CACHE_ALIGN;

	int MAX_BACK CACHE_ALIGN;
    // Guard node
    // Do not set this as const node
    Node guard CACHE_ALIGN;
} SimQueueStruct;

void SimQueueThreadStateInit(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid);
void SimQueueInit(SimQueueStruct *queue, int max_backoff);
void SimQueueEnqueue(SimQueueStruct *queue, SimQueueThreadState *th_state, ArgVal arg, int pid);
RetVal SimQueueDequeue(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid);

#endif
