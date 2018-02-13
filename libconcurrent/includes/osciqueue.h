#ifndef _OSCIQUEUE_H_
#define _OSCIQUEUE_H_

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <uthreads.h>
#include <types.h>
#include <osci.h>
#include <pool.h>
#include <queue-stack.h>

typedef struct OsciQueueStruct {
    OsciStruct enqueue_struct CACHE_ALIGN;
    OsciStruct dequeue_struct CACHE_ALIGN;
    volatile Node *last CACHE_ALIGN;
    volatile Node *first CACHE_ALIGN;
    Node guard CACHE_ALIGN;
    PoolStruct pool_node[FIBERS_GROUP] CACHE_ALIGN;
} OsciQueueStruct;


typedef struct OsciQueueThreadState {
    OsciThreadState enqueue_thread_state;
    OsciThreadState dequeue_thread_state;
} OsciQueueThreadState;


void OsciQueueInit(OsciQueueStruct *queue_object_struct);
void OsciQueueThreadStateInit(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, int pid);
void OsciQueueApplyEnqueue(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, ArgVal arg, int pid);
RetVal OsciQueueApplyDequeue(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, int pid);

#endif
