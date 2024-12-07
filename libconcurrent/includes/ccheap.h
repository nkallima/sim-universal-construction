/// @file ccheap.h
/// @author Nikolaos D. Kallimanis
/// @brief This file exposes the API of the CCHeap, which is a concurrent heap object of fixed size.
/// The provided implementation uses the dynamic serial heap implementation provided by `serialheap.h` combined with CCSynch combining object 
/// provided by `ccsynch.h`. This dynamic heap implementation is based on the static heap implementation provided
/// in https://github.com/ConcurrentDistributedLab/PersistentCombining repository. 
/// An example of use of this API is provided in benchmarks/ccheapbench.c file.
///
/// @copyright Copyright (c) 2024
#ifndef _CCHEAP_H_
#define _CCHEAP_H_

#include <limits.h>
#include <ccsynch.h>

#include <serialheap.h>


/// @brief The type of heap is max-heap 
#define CCHEAP_TYPE_MIN SYNCH_HEAP_TYPE_MIN
/// @brief The type of heap is min-heap
#define CCHEAP_TYPE_MAX SYNCH_HEAP_TYPE_MAX

/// @brief CCHeapStruct stores the state of an instance of the CCHeap concurrent heap implementation.
/// CCHeapStruct should be initialized using the CCHeapStructInit function.
typedef struct CCHeapStruct {
    /// @brief An instance of CCSynch.
    CCSynchStruct heap CACHE_ALIGN;
    /// @brief An instance of a serial heap implementation (see `serialheap.h`).
    SerialHeapStruct state; 
} CCHeapStruct;

/// @brief CCHeapThreadState stores each thread's local state for a single instance of CCHeap.
/// For each instance of CCHeap, a discrete instance of CCHeapThreadStateState should be used.
typedef struct CCHeapThreadState {
    /// @brief A CCSynchThreadState struct for the instance of CCSynch.
    CCSynchThreadState thread_state;
} CCHeapThreadState;

///  @brief This function initializes an instance of the CCHeap concurrent heap implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any operation on the heap object.
///  
///  @param heap_struct A pointer to an instance of the CCHeap concurrent heap implementation.
///  @param type Identifies the type of heap (i.e. min-heap or max-heap). In case that the argument is equal to CCHEAP_TYPE_MIN,
///  the type of heap is min. In case that the argument is equal to CCHEAP_TYPE_MAX, the type of heap is max.
///  @param nthreads The number of threads that will use the CCHeap concurrent heap implementation.
void CCHeapInit(CCHeapStruct *heap_struct, uint32_t type, uint32_t nthreads);

///  @brief This function should be called once by every thread before it applies any operation to the CCHeap concurrent heap implementation.
///  
///  @param heap_struct A pointer to an instance of the CCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of CCHeap.
///  @param pid The pid of the calling thread.
void CCHeapThreadStateInit(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, int pid);

///  @brief This function inserts a new element with value `arg` to the heap. 
///  
///  @param heap_struct A pointer to an instance of the CCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of CCHeap.
///  @param arg The value of the element that will be inserted in the heap.
///  @param pid The pid of the calling thread.
void CCHeapInsert(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, SynchHeapElement arg, int pid);

///  @brief This function removes the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the CCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of CCHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the removed element. In case that the heap is empty `EMPTY_HEAP` is returned.
SynchHeapElement CCHeapDeleteMin(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, int pid);

///  @brief This function returns (without removing) the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the CCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of CCHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the minimum element contained in the heap. In case that the heap is empty `EMPTY_HEAP` is returned. 
SynchHeapElement CCHeapGetMin(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, int pid);

#endif