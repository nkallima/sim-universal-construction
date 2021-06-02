/// @file dsmqueue.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the DSM-Queue concurrent queue implementation.
/// An example of use of this API is provided in benchmarks/dsmqueuebench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021
#ifndef _DSMQUEUE_H_
#define _DSMQUEUE_H_

#include <config.h>
#include <queue-stack.h>
#include <dsmsynch.h>
#include <primitives.h>

/// @brief DSMQueueStruct stores the state of an instance of the DSM-Queue concurrent queue implementation.
/// DSMQueueStruct should be initialized using the DSMQueueStructInit function.
typedef struct DSMQueueStruct {
    /// @brief A DSM-Synch instance for servicing the enqueue operations.
    DSMSynchStruct enqueue_struct CACHE_ALIGN;
    /// @brief A DSM-Synch instance for servicing the dequeue operations.
    DSMSynchStruct dequeue_struct CACHE_ALIGN;
    /// @brief A pointer to the last inserted element.
    volatile Node *last CACHE_ALIGN;
    /// @brief A pointer to the first inserted element.
    volatile Node *first CACHE_ALIGN;
    /// @brief A guard node that it is used only at the initialization of the queue.
    Node guard CACHE_ALIGN;
} DSMQueueStruct;

/// @brief DSMQueueThreadState stores each thread's local state for a single instance of DSM-Queue.
/// For each instance of DSM-Queue, a discrete instance of DSMQueueThreadState should be used.
typedef struct DSMQueueThreadState {
    /// @brief A DSMSynchThreadState for the instance of DSM-Synch that serves the enqueue operations.
    DSMSynchThreadState enqueue_thread_state;
    /// @brief A DSMSynchThreadState for the instance of DSM-Synch that serves the dequeue operations.
    DSMSynchThreadState dequeue_thread_state;
} DSMQueueThreadState;

/// @brief This function initializes an instance of the DSM-Queue concurrent queue implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param queue_object_struct A pointer to an instance of the DSM-Queue concurrent queue implementation.
/// @param nthreads The number of threads that will use the DSM-Queue concurrent queue implementation.
void DSMQueueStructInit(DSMQueueStruct *queue_object_struct, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the DSM-Queue concurrent queue implementation.
///
/// @param object_struct A pointer to an instance of the DSM-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of DSM-Queue.
/// @param pid The pid of the calling thread.
void DSMQueueThreadStateInit(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the DSM-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of DSM-Queue.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
/// @param pid The pid of the calling thread.
void DSMQueueApplyEnqueue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. dequeues) an element from the from of queue and returns its value.
///
/// @param object_struct A pointer to an instance of the DSM-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of DSM-Queue.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal DSMQueueApplyDequeue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid);

#endif
