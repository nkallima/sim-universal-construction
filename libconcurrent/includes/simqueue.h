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

typedef struct EnqState {
    ToggleVector applied;
    uint64_t copy_point;
    Node *tail CACHE_ALIGN;
    Node *first;
    Node *last;
#ifdef DEBUG
    int32_t counter;
#endif
    uint64_t __flex[1];
} EnqState;

#define EnqStateSize(N) (sizeof(EnqState) + _TVEC_VECTOR_SIZE(N))

typedef struct DeqState {
    ToggleVector applied;
    uint64_t copy_point;
    Node *head;
    RetVal *ret;
#ifdef DEBUG
    int32_t counter;
#endif
    uint64_t __flex[1];
} DeqState;

#define DeqStateSize(N) (sizeof(DeqState) + _TVEC_VECTOR_SIZE(N) + (N) * sizeof(RetVal))

// Each thread owns a private copy of the following variables
typedef struct SimQueueThreadState {
    ToggleVector mask;
    ToggleVector deq_toggle;
    ToggleVector my_deq_bit;
    ToggleVector enq_toggle;
    ToggleVector my_enq_bit;
    ToggleVector diffs;
    ToggleVector l_toggles;
    PoolStruct pool_node;
    int deq_local_index;
    int enq_local_index;
    int backoff;
} SimQueueThreadState;

typedef struct SimQueueStruct {
    volatile pointer_t enq_sp CACHE_ALIGN;
    volatile pointer_t deq_sp CACHE_ALIGN;
    // Guard node
    // Do not set this as const node
    Node guard CACHE_ALIGN;
    ToggleVector enqueuers CACHE_ALIGN;
    ToggleVector dequeuers;
    ArgVal *announce;
    EnqState **enq_pool;
    DeqState **deq_pool;
    uint32_t nthreads;
    int MAX_BACK;
} SimQueueStruct;

void SimQueueInit(SimQueueStruct *queue, uint32_t nthreads, int max_backoff);
void SimQueueThreadStateInit(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid);
void SimQueueEnqueue(SimQueueStruct *queue, SimQueueThreadState *th_state, ArgVal arg, int pid);
RetVal SimQueueDequeue(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid);

#endif
