/// @file fcqueue.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of a concurrent queue implementation using the flat-combining synchronization technique (or shortly FC).
/// The provided code of flat-combining is an alternative implementation of the Synch framework and it has NO relation with the original code of flat-combining provided in
/// https://github.com/mit-carbon/Flat-Combining, since it is written from the scratch.
/// In many cases, this code performs much better than the original flat-combining implementation. An example of use of this API is provided in benchmarks/fcqueuebench.c file.
///
/// For a more detailed description of flat-combining, see the original publication:
/// Danny Hendler, Itai Incze, Nir Shavit, and Moran Tzafrir. Flat combining and the synchronization-parallelism tradeoff. 
/// In Proceedings of the twenty-second annual ACM symposium on Parallelism in algorithms and architectures (SPAA 2010), pp. 355-364.
/// @copyright Copyright (c) 2021
#ifndef _FCQUEUE_H_
#define _FCQUEUE_H_

#include <fc.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

/// @brief FCQueueStruct stores the state of an instance of the FC-Queue concurrent queue implementation.
/// FCQueueStruct should be initialized using the FCQueueStructInit function.
typedef struct FCQueueStruct {
    /// @brief An FC instance for servicing the enqueue operations.
    FCStruct enqueue_struct CACHE_ALIGN;
    /// @brief An FC instance for servicing the dequeue operations.
    FCStruct dequeue_struct CACHE_ALIGN;
    /// @brief A pointer to the last inserted element.
    volatile Node *last CACHE_ALIGN;
    /// @brief A pointer to the first inserted element.
    volatile Node *first CACHE_ALIGN;
    /// @brief A guard node that it is used only at the initialization of the queue.
    Node guard CACHE_ALIGN;
} FCQueueStruct;

/// @brief FCQueueThreadState stores each thread's local state for a single instance of FC-Queue.
/// For each instance of FC-Queue, a discrete instance of FCQueueThreadState should be used.
typedef struct FCQueueThreadState {
    /// @brief A FCThreadState struct for the instance of flat-combining that serves the enqueue operations.
    FCThreadState enqueue_thread_state;
    /// @brief A FCThreadState struct for the instance of flat-combining that serves the dequeue operations.
    FCThreadState dequeue_thread_state;
} FCQueueThreadState;

/// @brief This function initializes an instance of the FC-Queue concurrent queue implementation.
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param queue_object_struct A pointer to an instance of the FC-Queue concurrent queue implementation.
/// @param nthreads The number of threads that will use the FC-Queue concurrent queue implementation.
void FCQueueStructInit(FCQueueStruct *queue_object_struct, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the FC-Queue concurrent queue implementation.
///
/// @param object_struct A pointer to an instance of the FC-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of FC-Queue.
/// @param pid The pid of the calling thread.
void FCQueueThreadStateInit(FCQueueStruct *object_struct, FCQueueThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the FC-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of FC-Queue.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
/// @param pid The pid of the calling thread.
void FCQueueApplyEnqueue(FCQueueStruct *object_struct, FCQueueThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. dequeues) an element from the front of the queue and returns its value.
///
/// @param object_struct A pointer to an instance of the FC-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of FC-Queue.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal FCQueueApplyDequeue(FCQueueStruct *object_struct, FCQueueThreadState *lobject_struct, int pid);

#endif
