/// @file fcheap.h
/// @author Nikolaos D. Kallimanis
/// @brief This file exposes the API of the FCHeap, which is a concurrent heap object of fixed size.
/// The provided implementation uses the dynamic serial heap implementation provided by `serialheap.h` combined with the flat-combining object 
/// provided by `fc.h`. This dynamic heap implementation is based on the static heap implementation provided
/// in https://github.com/ConcurrentDistributedLab/PersistentCombining repository. 
/// An example of use of this API is provided in benchmarks/fcheapbench.c file.
///
/// @copyright Copyright (c) 2024
#ifndef _FCHEAP_H_
#define _FCHEAP_H_

#include <limits.h>
#include <fc.h>

#include <serialheap.h>


/// @brief The type of heap is max-heap 
#define FCHEAP_TYPE_MIN SYNCH_HEAP_TYPE_MIN
/// @brief The type of heap is min-heap
#define FCHEAP_TYPE_MAX SYNCH_HEAP_TYPE_MAX

/// @brief FCHeapStruct stores the state of an instance of the FCHeap concurrent heap implementation.
/// FCHeapStruct should be initialized using the FCHeapStructInit function.
typedef struct FCHeapStruct {
    /// @brief An instance of FC.
    FCStruct heap CACHE_ALIGN;
    /// @brief An instance of a serial heap implementation (see `serialheap.h`).
    SerialHeapStruct state; 
} FCHeapStruct;

/// @brief FCHeapThreadState stores each thread's local state for a single instance of FCHeap.
/// For each instance of FCHeap, a discrete instance of FCHeapThreadStateState should be used.
typedef struct FCHeapThreadState {
    /// @brief A FCThreadState struct for the instance of FC.
    FCThreadState thread_state;
} FCHeapThreadState;

///  @brief This function initializes an instance of the FCHeap concurrent heap implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any operation on the heap object.
///  
///  @param heap_struct A pointer to an instance of the FCHeap concurrent heap implementation.
///  @param type Identifies the type of heap (i.e. min-heap or max-heap). In case that the argument is equal to FCHEAP_TYPE_MIN,
///  the type of heap is min. In case that the argument is equal to FCHEAP_TYPE_MAX, the type of heap is max.
///  @param nthreads The number of threads that will use the FCHeap concurrent heap implementation.
void FCHeapInit(FCHeapStruct *heap_struct, uint32_t type, uint32_t nthreads);

///  @brief This function should be called once by every thread before it applies any operation to the FCHeap concurrent heap implementation.
///  
///  @param heap_struct A pointer to an instance of the FCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of FCHeap.
///  @param pid The pid of the calling thread.
void FCHeapThreadStateInit(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, int pid);

///  @brief This function inserts a new element with value `arg` to the heap. 
///  
///  @param heap_struct A pointer to an instance of the FCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of FCHeap.
///  @param arg The value of the element that will be inserted in the heap.
///  @param pid The pid of the calling thread.
void FCHeapInsert(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, SynchHeapElement arg, int pid);

///  @brief This function removes the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the FCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of FCHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the removed element. In case that the heap is empty `SYNCH_HEAP_EMPTY` is returned.
SynchHeapElement FCHeapDeleteMin(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, int pid);

///  @brief This function returns (without removing) the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the FCHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of FCHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the minimum element contained in the heap. In case that the heap is empty `SYNCH_HEAP_EMPTY` is returned. 
SynchHeapElement FCHeapGetMin(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, int pid);

#endif