/// @file hqueue.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the H-Queue concurrent queue implementation.
/// An example of use of this API is provided in benchmarks/hqueuebench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021
#ifndef _HQUEUE_H_
#define _HQUEUE_H_

#include <config.h>
#include <hsynch.h>
#include <queue-stack.h>
#include <primitives.h>

/// @brief HQueueStruct stores the state of an instance of the H-Queue concurrent queue implementation.
/// HQueueStruct should be initialized using the HQueueStructInit function.
typedef struct HQueueStruct {
    /// @brief A H-Synch instance for servicing the enqueue operations.
    HSynchStruct *enqueue_struct;
    /// @brief A H-Synch instance for servicing the dequeue operations.
    HSynchStruct *dequeue_struct;
    /// @brief A pointer to the last inserted element.
    volatile Node *last CACHE_ALIGN;
    /// @brief A pointer to the first inserted element.
    volatile Node *first CACHE_ALIGN;
    /// @brief A guard node that it is used only at the initialization of the queue.
    Node guard CACHE_ALIGN;
} HQueueStruct;

/// @brief HQueueThreadState stores each thread's local state for a single instance of H-Queue.
/// For each instance of H-Queue, a discrete instance of HQueueThreadState should be used.
typedef struct HQueueThreadState {
    /// @brief A HSynchThreadState for the instance of H-Synch that serves the enqueue operations.
    HSynchThreadState enqueue_thread_state CACHE_ALIGN;
    /// @brief A HSynchThreadState for the instance of H-Synch that serves the dequeue operations.
    HSynchThreadState dequeue_thread_state CACHE_ALIGN;
} HQueueThreadState;

/// @brief This function initializes an instance of the H-Queue concurrent queue implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param queue_object_struct A pointer to an instance of the H-Queue concurrent queue implementation.
/// @param nthreads The number of threads that will use the H-Queue concurrent queue implementation.
/// @param numa_nodes The number of Numa nodes (which may differ with the actual hw numa nodes) that H-Queue should consider.
/// In case that numa_nodes is equal to HSYNCH_DEFAULT_NUMA_POLICY, the number of Numa nodes provided by the HW is used
/// (see more on hsynch.h).
void HQueueInit(HQueueStruct *queue_object_struct, uint32_t nthreads, uint32_t numa_nodes);

/// @brief This function should be called once before the thread applies any operation to the H-Queue concurrent queue implementation.
///
/// @param object_struct A pointer to an instance of the H-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of H-Queue.
/// @param pid The pid of the calling thread.
void HQueueThreadStateInit(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the H-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of H-Queue.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
/// @param pid The pid of the calling thread.
void HQueueApplyEnqueue(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. dequeues) an element from the from of queue and returns its value.
///
/// @param object_struct A pointer to an instance of the H-Queue concurrent queue implementation.
/// @param lobject_struct A pointer to thread's local state of H-Queue.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal HQueueApplyDequeue(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, int pid);

#endif
