/// @file hheap.h
/// @author Nikolaos D. Kallimanis
/// @brief This file exposes the API of the HSynchHeap, which is a concurrent heap object of fixed size.
/// The provided implementation uses the serial dynamic heap implementation provided by `serialheap.h` combined with HSynch combining object 
/// provided by `hsynch.h`. /// This dynamic heap implementation is based on the static heap implementation provided
/// in https://github.com/ConcurrentDistributedLab/PersistentCombining repository.
/// An example of use of this API is provided in benchmarks/hheapbench.c file.
///
/// @copyright Copyright (c) 2024
#ifndef _HHEAP_H_
#define _HHEAP_H_

#include <limits.h>
#include <hsynch.h>

#include <serialheap.h>

/// @brief The type of heap is max-heap 
#define HHEAP_TYPE_MIN SYNCH_HEAP_TYPE_MIN
/// @brief The type of heap is min-heap
#define HHEAP_TYPE_MAX SYNCH_HEAP_TYPE_MAX

/// @brief HSynchHeapStruct stores the state of an instance of the HSynchHeap concurrent heap implementation.
/// HSynchHeapStruct should be initialized using the HSynchHeapStructInit function.
typedef struct HSynchHeapStruct {
    /// @brief An instance of HSynch.
    HSynchStruct heap CACHE_ALIGN;
    /// @brief An instance of a serial heap implementation (see `serialheap.h`).
    SerialHeapStruct state; 
} HSynchHeapStruct;

/// @brief HSynchHeapThreadState stores each thread's local state for a single instance of HSynchHeap.
/// For each instance of HSynchHeap, a discrete instance of HSynchHeapThreadStateState should be used.
typedef struct HSynchHeapThreadState {
    /// @brief A HSynchThreadState struct for the instance of HSynch.
    HSynchThreadState thread_state;
} HSynchHeapThreadState;

///  @brief This function initializes an instance of the HSynchHeap concurrent heap implementation.
///
///  This function should be called once (by a single thread) before any other thread tries to
///  apply any operation on the heap object.
///  
///  @param heap_struct A pointer to an instance of the HSynchHeap concurrent heap implementation.
///  @param type Identifies the type of heap (i.e. min-heap or max-heap). In case that the argument is equal to HHEAP_TYPE_MIN,
///  the type of heap is min. In case that the argument is equal to HHEAP_TYPE_MAX, the type of heap is max.
///  @param nthreads The number of threads that will use the HSynchHeap concurrent heap implementation.
///  @param numa_nodes param numa_nodes The number of Numa nodes (which may differ with the actual hw numa nodes) that H-Stack should consider.
///  In case that numa_nodes is equal to HSYNCH_DEFAULT_NUMA_POLICY, the number of Numa nodes provided by the HW is used
///  (see more on hsynch.h).
void HSynchHeapInit(HSynchHeapStruct *heap_struct, uint32_t type, uint32_t nthreads, uint32_t numa_nodes);

///  @brief This function should be called once by every thread before it applies any operation to the HSynchHeap concurrent heap implementation.
///  
///  @param heap_struct A pointer to an instance of the HSynchHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of HSynchHeap.
///  @param pid The pid of the calling thread.
void HSynchHeapThreadStateInit(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, int pid);

///  @brief This function inserts a new element with value `arg` to the heap. 
///  
///  @param heap_struct A pointer to an instance of the HSynchHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of HSynchHeap.
///  @param arg The value of the element that will be inserted in the heap.
///  @param pid The pid of the calling thread.
void HSynchHeapInsert(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, SynchHeapElement arg, int pid);

///  @brief This function removes the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the HSynchHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of HSynchHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the removed element. In case that the heap is empty `EMPTY_HEAP` is returned.
SynchHeapElement HSynchHeapDeleteMin(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, int pid);

///  @brief This function returns (without removing) the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the HSynchHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of HSynchHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the minimum element contained in the heap. In case that the heap is empty `EMPTY_HEAP` is returned. 
SynchHeapElement HSynchHeapGetMin(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, int pid);

#endif