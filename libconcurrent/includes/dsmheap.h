/// @file dsmheap.h
/// @author Nikolaos D. Kallimanis
/// @brief This file exposes the API of the DSMHeap, which is a concurrent heap object of fixed size.
/// The provided implementation uses the dynamic serial heap implementation provided by `serialheap.h` combined with DSMSynch combining object 
/// provided by `dsmsynch.h`. /// This dynamic heap implementation is based on the static heap implementation provided 
/// in https://github.com/ConcurrentDistributedLab/PersistentCombining repository. 
/// An example of use of this API is provided in benchmarks/dsmheapbench.c file.
///
/// @copyright Copyright (c) 2024
#ifndef _DSMHEAP_H_
#define _DSMHEAP_H_

#include <limits.h>
#include <dsmsynch.h>

#include <serialheap.h>

/// @brief The type of heap is max-heap 
#define DSMHEAP_TYPE_MIN SYNCH_HEAP_TYPE_MIN
/// @brief The type of heap is min-heap
#define DSMHEAP_TYPE_MAX SYNCH_HEAP_TYPE_MAX

/// @brief DSMHeapStruct stores the state of an instance of the DSMHeap concurrent heap implementation.
/// DSMHeapStruct should be initialized using the DSMHeapStructInit function.
typedef struct DSMHeapStruct {
    /// @brief An instance of DSMSynch.
    DSMSynchStruct heap CACHE_ALIGN;
    /// @brief An instance of a serial heap implementation (see `serialheap.h`).
    SerialHeapStruct state; 
} DSMHeapStruct;

/// @brief DSMHeapThreadState stores each thread's local state for a single instance of DSMHeap.
/// For each instance of DSMHeap, a discrete instance of DSMHeapThreadStateState should be used.
typedef struct DSMHeapThreadState {
    /// @brief A DSMSynchThreadState struct for the instance of DSMSynch.
    DSMSynchThreadState thread_state;
} DSMHeapThreadState;

///  @brief This function initializes an instance of the DSMHeap concurrent heap implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any operation on the heap object.
///  
///  @param heap_struct A pointer to an instance of the DSMHeap concurrent heap implementation.
///  @param type Identifies the type of heap (i.e. min-heap or max-heap). In case that the argument is equal to DSMHEAP_TYPE_MIN,
///  the type of heap is min. In case that the argument is equal to DSMHEAP_TYPE_MAX, the type of heap is max.
///  @param nthreads The number of threads that will use the DSMHeap concurrent heap implementation.
void DSMHeapInit(DSMHeapStruct *heap_struct, uint32_t type, uint32_t nthreads);

///  @brief This function should be called once by every thread before it applies any operation to the DSMHeap concurrent heap implementation.
///  
///  @param heap_struct A pointer to an instance of the DSMHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of DSMHeap.
///  @param pid The pid of the calling thread.
void DSMHeapThreadStateInit(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, int pid);

///  @brief This function inserts a new element with value `arg` to the heap. 
///  
///  @param heap_struct A pointer to an instance of the DSMHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of DSMHeap.
///  @param arg The value of the element that will be inserted in the heap.
///  @param pid The pid of the calling thread.
void DSMHeapInsert(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, SynchHeapElement arg, int pid);

///  @brief This function removes the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the DSMHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of DSMHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the removed element. In case that the heap is empty `EMPTY_HEAP` is returned.
SynchHeapElement DSMHeapDeleteMin(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, int pid);

///  @brief This function returns (without removing) the element of the heap that has the minimum value.
///  
///  @param heap_struct A pointer to an instance of the DSMHeap concurrent heap implementation.
///  @param lobject_struct A pointer to thread's local state of DSMHeap.
///  @param pid The pid of the calling thread.
///  @return The value of the minimum element contained in the heap. In case that the heap is empty `EMPTY_HEAP` is returned. 
SynchHeapElement DSMHeapGetMin(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, int pid);

#endif