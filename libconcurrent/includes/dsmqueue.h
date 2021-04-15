#ifndef _DSMQUEUE_H_
#define _DSMQUEUE_H_

#include <config.h>
#include <queue-stack.h>
#include <dsmsynch.h>
#include <primitives.h>

typedef struct DSMQueueStruct {
    DSMSynchStruct enqueue_struct CACHE_ALIGN;
    DSMSynchStruct dequeue_struct CACHE_ALIGN;
    volatile Node *last CACHE_ALIGN;
    volatile Node *first CACHE_ALIGN;
    Node guard CACHE_ALIGN;
} DSMQueueStruct;

typedef struct DSMQueueThreadState {
    DSMSynchThreadState enqueue_thread_state;
    DSMSynchThreadState dequeue_thread_state;
} DSMQueueThreadState;

void DSMQueueStructInit(DSMQueueStruct *queue_object_struct, uint32_t nthreads);
void DSMQueueThreadStateInit(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid);
void DSMQueueApplyEnqueue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, ArgVal arg, int pid);
RetVal DSMQueueApplyDequeue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid);

#endif
