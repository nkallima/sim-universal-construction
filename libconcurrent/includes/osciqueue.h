/// @file osciqueue.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the OsciQueue concurrent queue implementation.
/// An example of use of this API is provided in benchmarks/osciqueuebench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis. "Lock Oscillation: Boosting the Performance of Concurrent Data Structures",
/// In Proceedings of the 21st International Conference on Principles of Distributed Systems (Opodis), 2017.
/// @copyright Copyright (c) 2021
#ifndef _OSCIQUEUE_H_
#define _OSCIQUEUE_H_

#include <config.h>
#include <primitives.h>
#include <osci.h>
#include <pool.h>
#include <queue-stack.h>

/// @brief OsciQueueStruct stores the state of an instance of the OsciQueue concurrent queue implementation.
/// OsciQueueStruct should be initialized using the OsciQueueStructInit function.
typedef struct OsciQueueStruct {
    /// @brief An Osci instance for servicing the enqueue operations.
    OsciStruct enqueue_struct CACHE_ALIGN;
    /// @brief An Osci instance for servicing the dequeue operations.
    OsciStruct dequeue_struct CACHE_ALIGN;
    /// @brief A pointer to the last inserted element.
    volatile Node *last CACHE_ALIGN;
    /// @brief A pointer to the first inserted element.
    volatile Node *first CACHE_ALIGN;
    /// @brief A guard node that it is used only at the initialization of the queue.
    Node guard CACHE_ALIGN;
    /// @brief Pointer to an array of pools of nodes (a single pool per fiber). It is used for fast node allocation on enqueue operations.
    PoolStruct *pool_node CACHE_ALIGN;
} OsciQueueStruct;

/// @brief OsciQueueThreadState stores each thread's local state for a single instance of OsciQueue.
/// For each instance of OsciQueue, a discrete instance of OsciQueueThreadState should be used.
typedef struct OsciQueueThreadState {
    /// @brief An OsciThreadState struct for the instance of Osci that serves the enqueue operations.
    OsciThreadState enqueue_thread_state;
    /// @brief An OsciThreadState struct for the instance of Osci that serves the dequeue operations.
    OsciThreadState dequeue_thread_state;
} OsciQueueThreadState;

/// @brief This function initializes an instance of the OsciQueue concurrent queue implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param queue_object_struct A pointer to an instance of the OsciQueue concurrent queue implementation.
/// @param nthreads The number of threads that will use the OsciQueue concurrent queue implementation.
/// @param fibers_per_thread The number of fibers per Posix thread.
void OsciQueueInit(OsciQueueStruct *queue_object_struct, uint32_t nthreads, uint32_t fibers_per_thread);

/// @brief This function should be called once before the thread applies any operation to the OsciQueue concurrent queue implementation.
///
/// @param object_struct A pointer to an instance of the OsciQueue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of OsciQueue.
/// @param pid The pid of the calling thread.
void OsciQueueThreadStateInit(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the OsciQueue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of OsciQueue.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
/// @param pid The pid of the calling thread.
void OsciQueueApplyEnqueue(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. dequeues) an element from the front of the queue and returns its value.
///
/// @param object_struct A pointer to an instance of the OsciQueue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of OsciQueue.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal OsciQueueApplyDequeue(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, int pid);

#endif
