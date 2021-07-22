/// @file simqueue.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the SimQueue concurrent queue implementation.
/// An example of use of this API is provided in benchmarks/simqueuebench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis. "A highly-efficient wait-free universal construction". 
/// Proceedings of the twenty-third annual ACM symposium on Parallelism in algorithms and architectures (SPAA), 2011.
/// @copyright Copyright (c) 2021
#ifndef _SIMQUEUE_H_
#define _SIMQUEUE_H_

#include <config.h>
#include <primitives.h>
#include <queue-stack.h>
#include <sim.h>
#include <pool.h>

/// @brief This struct stores the data about the state of the enqueue instance. It is used by the threads that perform enqueue operations.
typedef struct EnqState {
    /// @brief The applied vector of toggles.
    ToggleVector applied;
    /// @brief In case you want to copy this struct, start from here.
    uint64_t copy_point;
    /// @brief A pointer to the tail of the queue.
    Node *tail CACHE_ALIGN;
    /// @brief A pointer to the first element of the last batch of applied enqueue operations.
    Node *first;
    /// @brief A pointer to the last element of the last batch of applied enqueue operations.
    Node *last;
#ifdef DEBUG
    int32_t counter;
#endif
    uint64_t __flex[1];
} EnqState;

/// @brief A macro for calculating the size of the EnqState struct for a specific amount of threads.
#define EnqStateSize(N) (sizeof(EnqState) + _TVEC_VECTOR_SIZE(N))

/// @brief This struct stores the data for the state of the dequeue instance. Used by the threads that perform dequeue operations.
typedef struct DeqState {
    /// @brief The applied vector of toggles.
    ToggleVector applied;
    /// @brief In case you want to copy this struct, start from here.
    uint64_t copy_point;
    /// @brief A pointer to the head of the queue.
    Node *head;
    /// @brief A pointer to the array of return values.
    RetVal *ret;
#ifdef DEBUG
    int32_t counter;
#endif
    uint64_t __flex[1];
} DeqState;

/// @brief A macro for calculating the size of the DeqState struct for a specific amount of threads.
#define DeqStateSize(N) (sizeof(DeqState) + _TVEC_VECTOR_SIZE(N) + (N) * sizeof(RetVal))

/// @brief SimQueueThreadState stores each thread's local state for a single instance of SimQueue.
/// For each instance of SimQueue, a discrete instance of SimQueueThreadState should be used.
typedef struct SimQueueThreadState {
    ToggleVector mask;
    ToggleVector deq_toggle;
    ToggleVector my_deq_bit;
    ToggleVector enq_toggle;
    ToggleVector my_enq_bit;
    ToggleVector diffs;
    ToggleVector l_toggles;
    /// @brief A pool of Node structs used for fast allocation during enqueue operations.
    PoolStruct pool_node;
    /// @brief The next available free copy of EnqState that could be used on an enqueue operation.
    int deq_local_index;
    /// @brief The next available free copy of DeqState that could be used on a dequeue operation.
    int enq_local_index;
    /// @brief The maximum backoff value.
    int max_backoff;
} SimQueueThreadState;

/// @brief SimQueueStruct stores the state of an instance of the SimQueue.
/// SimQueueStruct should be initialized using the SimQueueStructInit function.
typedef struct SimQueueStruct {
    /// @brief Pointer to a EnqState struct that contains the most recent and valid copy of the SimQueue instance used by the enqueue operations.
    volatile pointer_t enq_sp CACHE_ALIGN;
    /// @brief Pointer to a DeqState struct that contains the most recent and valid copy of the SimQueue instance used by the dequeue operations.
    volatile pointer_t deq_sp CACHE_ALIGN;
    /// @brief This queue implementation uses a guard node.
    Node guard CACHE_ALIGN;
    /// @brief Toggle bits for the enqueue operations.
    ToggleVector enqueuers CACHE_ALIGN;
    /// @brief Toggle bits for the dequeue operations.
    ToggleVector dequeuers;
    /// @brief Pointer to an array, where threads announce only the enqueue requests that want to perform to the object.
    ArgVal *announce;
    /// @brief An array of pools (one pool per thread) of EnqState structs, used by the enqueuers.
    EnqState **enq_pool;
    /// @brief An array of pools (one pool per thread) of DeqState structs, used by dequeuers.
    DeqState **deq_pool;
    /// @brief The number of threads that use this instance of SimQueue.
    uint32_t nthreads;
    /// @brief The maximum backoff value.
    int MAX_BACK;
} SimQueueStruct;

/// @brief This function initializes an instance of the SimQueue concurrent queue implementation.
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param queue A pointer to an instance of the SimQueue concurrent queue implementation.
/// @param nthreads The number of threads that will use the SimQueue concurrent queue implementation.
/// @param max_backoff The maximum value for backoff (usually this is much lower than 100).
void SimQueueStructInit(SimQueueStruct *queue, uint32_t nthreads, int max_backoff);

/// @brief This function should be called once before the thread applies any operation to the SimQueue concurrent queue implementation.
///
/// @param queue A pointer to an instance of the SimQueue concurrent queue implementation.
/// @param th_state A pointer to thread's local state of SimQueue.
/// @param pid The pid of the calling thread.
void SimQueueThreadStateInit(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param queue A pointer to an instance of the SimQueue concurrent queue implementation.
/// @param th_state A pointer to thread's local state of SimQueue.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
/// @param pid The pid of the calling thread.
void SimQueueEnqueue(SimQueueStruct *queue, SimQueueThreadState *th_state, ArgVal arg, int pid);

/// @brief This function removes (i.e. dequeues) an element from the front of the queue and returns its value.
///
/// @param queue A pointer to an instance of the SimQueue concurrent queue implementation.
/// @param th_state A pointer to thread's local state of SimQueue.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal SimQueueDequeue(SimQueueStruct *queue, SimQueueThreadState *th_state, int pid);

#endif
