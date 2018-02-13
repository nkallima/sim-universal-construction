#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include <ccsynch.h>
#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <pool.h>
#include <queue-stack.h>


typedef struct CCQueueStruct {
    CCSynchStruct enqueue_struct CACHE_ALIGN;
    CCSynchStruct dequeue_struct CACHE_ALIGN;
    volatile Node *last CACHE_ALIGN;
    volatile Node *first CACHE_ALIGN;
    Node guard CACHE_ALIGN;
} CCQueueStruct;


typedef struct CCQueueThreadState {
    CCSynchThreadState enqueue_thread_state;
    CCSynchThreadState dequeue_thread_state;
} CCQueueThreadState;


void CCQueueStructInit(CCQueueStruct *queue_object_struct);
void CCQueueThreadStateInit(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, int pid);
void CCQueueApplyEnqueue(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, ArgVal arg, int pid);
RetVal CCQueueApplyDequeue(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, int pid);

#endif
