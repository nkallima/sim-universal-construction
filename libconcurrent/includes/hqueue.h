#ifndef _HQUEUE_H_
#define _HQUEUE_H_

#include <hsynch.h>
#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <pool.h>
#include <queue-stack.h>


typedef struct HQueueStruct {
    HSynchStruct enqueue_struct CACHE_ALIGN;
    HSynchStruct dequeue_struct CACHE_ALIGN;
    volatile Node *last CACHE_ALIGN;
    volatile Node *first CACHE_ALIGN;
    Node guard CACHE_ALIGN;
} HQueueStruct;


typedef struct HQueueThreadState {
    HSynchThreadState enqueue_thread_state;
    HSynchThreadState dequeue_thread_state;
} HQueueThreadState;


void HQueueInit(HQueueStruct *queue_object_struct, uint32_t nthreads, uint32_t numa_nodes);
void HQueueThreadStateInit(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, int pid);
void HQueueApplyEnqueue(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, ArgVal arg, int pid);
RetVal HQueueApplyDequeue(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, int pid);

#endif
