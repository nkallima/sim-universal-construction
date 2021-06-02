/// @file ccqueue.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the CC-Queue concurrent queue implementation.
/// An example of use of this API is provided in benchmarks/ccqueuebench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021

#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include <config.h>
#include <queue-stack.h>
#include <ccsynch.h>

/// @brief CCQueueStruct stores the state of an instance of the CC-Queue concurrent queue implementation.
/// CCQueueStruct should be initialized using the CCQueueStructInit function.
typedef struct CCQueueStruct {
    /// @brief A CC-Synch instance for servicing the enqueue operations.
    CCSynchStruct enqueue_struct CACHE_ALIGN;
    /// @brief A CC-Synch instance for servicing the dequeue operations.
    CCSynchStruct dequeue_struct CACHE_ALIGN;
    /// @brief A pointer to the last inserted element.
    volatile Node *last CACHE_ALIGN;
    /// @brief A pointer to the first inserted element.
    volatile Node *first CACHE_ALIGN;
    /// @brief A guard node that it is used only at the initialization of the queue.
    Node guard CACHE_ALIGN;
} CCQueueStruct;

/// @brief CCQueueThreadState stores each thread's local state for a single instance of CC-Queue.
/// For each instance of CC-Queue, a discrete instance of CCQueueThreadState should be used.
typedef struct CCQueueThreadState {
    /// @brief A CCSynchThreadState for the instance of CC-Synch that serves the enqueue operations.
    CCSynchThreadState enqueue_thread_state;
    /// @brief A CCSynchThreadState for the instance of CC-Synch that serves the dequeue operations.
    CCSynchThreadState dequeue_thread_state;
} CCQueueThreadState;

/// @brief This function initializes an instance of the CC-Queue concurrent queue implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param queue_object_struct A pointer to an instance of the CC-Queue concurrent queue implementation.
/// @param nthreads The number of threads that will use the CC-Queue concurrent queue implementation.
void CCQueueStructInit(CCQueueStruct *queue_object_struct, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the CC-Queue concurrent queue implementation.
///
/// @param object_struct A pointer to an instance of the CC-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of CC-Queue.
/// @param pid The pid of the calling thread.
void CCQueueThreadStateInit(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the CC-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of CC-Queue.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
/// @param pid The pid of the calling thread.
void CCQueueApplyEnqueue(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. dequeues) an element from the from of queue and returns its value.
///
/// @param object_struct A pointer to an instance of the CC-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of CC-Queue.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal CCQueueApplyDequeue(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, int pid);

#endif
