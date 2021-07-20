/// @file msqueue.h
/// @brief This file exposes the API of the MS-Queue concurrent queue implementation.
/// An example of use of this API is provided in benchmarks/msqueuebench.c file.
///
/// For a more detailed description see the original publication:
/// Maged M. Michael, and Michael L. Scott. "Simple, fast, and practical non-blocking and blocking concurrent queue algorithms". 
/// Proceedings of the fifteenth annual ACM symposium on Principles of distributed computing. ACM, 1996.
#ifndef _MSQUEUE_H_
#define _MSQUEUE_H_

#include <config.h>
#include <primitives.h>
#include <backoff.h>
#include <pool.h>
#include <queue-stack.h>

/// @brief MSQueueStruct stores the state of an instance of the MS-Queue concurrent queue implementation.
/// MSQueueStruct should be initialized using the MSQueueStructInit function.
typedef struct MSQueueStruct {
    /// @brief A pointer to the head element of the queue.
    volatile Node *head CACHE_ALIGN;
    /// @brief A pointer to the tail element of the queue.
    volatile Node *tail CACHE_ALIGN;
} MSQueueStruct;

/// @brief MSQueueThreadState stores each thread's local state for a single instance of MS-Queue.
/// For each instance of MS-Queue, a discrete instance of MSQueueThreadState should be used.
typedef struct MSQueueThreadState {
    /// @brief A pool object per thread is used for fast memory allocation.
    PoolStruct pool;
    /// @brief A backoff object per thread is used for reducing the contention while accessing the queue.
    BackoffStruct backoff;
} MSQueueThreadState;


/// @brief This function initializes an instance of the MS-Queue concurrent queue implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param l A pointer to an instance of the MS-Queue concurrent queue implementation.
/// @param nthreads The number of threads that will use the MS-Queue concurrent queue implementation.
void MSQueueInit(MSQueueStruct *l);

/// @brief This function should be called once before the thread applies any operation to the MS-Queue concurrent queue implementation.
///
/// @param th_state A pointer to thread's local state of MS-Queue.
/// @param min_back The minimum value for backoff (in most cases 0 is a good start).
/// @param max_back The maximum value for backoff (usually this is much lower than 100).
void MSQueueThreadStateInit(MSQueueThreadState *th_state, int min_back, int max_back);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param l A pointer to an instance of the MS-Queue concurrent queue implementation.
/// @param th_state A pointer to thread's local state of MS-Queue.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
void MSQueueEnqueue(MSQueueStruct *l, MSQueueThreadState *th_state, ArgVal arg);

/// @brief This function removes (i.e. dequeues) an element from the front of the queue and returns its value.
///
/// @param l A pointer to an instance of the MS-Queue concurrent queue implementation.
/// @param th_state A pointer to thread's local state of MS-Queue.
/// @return The value of the removed element.
RetVal MSQueueDequeue(MSQueueStruct *l, MSQueueThreadState *th_state);

#endif