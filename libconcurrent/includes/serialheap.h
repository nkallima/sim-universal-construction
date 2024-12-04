/// @file serialheap.h
/// @author Nikolaos D. Kallimanis
/// @brief This file exposes the API of the a serial heap implementation of fixed size. This heap implementation 
/// supports operations for inserting elements, removing the minimum element and getting the value minimum element.
/// It is based on the static heap implementation provided in https://github.com/ConcurrentDistributedLab/PersistentCombining. 
/// This serial implementation could be combined with almost any combining object provided by the Synch framework
/// (e.g. CC-Synch, Osci, H-Synch, etc) in order to implement fast and efficient concurrent heap implementations.
///
/// @copyright Copyright (c) 2024
#ifndef _SERIALHEAP_H_
#define _SERIALHEAP_H_

#include <primitives.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define SYNCH_HEAP_EMPTY             LLONG_MIN
#define SYNCH_HEAP_EMPTY_NODE        LLONG_MIN
#define SYNCH_HEAP_INSERT_SUCCESS    0
#define SYNCH_HEAP_INSERT_FAIL       (LLONG_MIN + 1)
#define SYNCH_HEAP_MAX_LEVELS        32
#define SYNCH_HEAP_MAX_SIZE          (1ULL << (SYNCH_HEAP_MAX_LEVELS))
#define SYNCH_HEAP_INITIAL_SIZE      (1ULL << (10))
#define SYNCH_HEAP_INITIAL_LEVELS    10ULL
#define SYNCH_HEAP_SIZE_OF_LEVEL(L)  (1ULL << (L))
#define SYNCH_HEAP_INSERT_OP         0x1000000000000000ULL
#define SYNCH_HEAP_DELETE_MIN_MAX_OP 0x2000000000000000ULL
#define SYNCH_HEAP_GET_MIN_MAX_OP    0x3000000000000000ULL
#define SYNCH_HEAP_OP_MASK           0x7000000000000000ULL
#define SYNCH_HEAP_VAL_MASK          (~(SYNCH_HEAP_OP_MASK))
#define SYNCH_HEAP_TYPE_MIN          0x0
#define SYNCH_HEAP_TYPE_MAX          0x1
#define SynchHeapElement             uint64_t

/// @brief SerialHeapStruct stores the state of an instance of the serial heap implementation. 
/// This heap data-structure is implemented using an array of fixed size.
/// This struct should be initialized using the serialHeapInit function.
typedef struct SerialHeapStruct {
    /// @brief The total number of items that the heap currently contains.
    volatile uint64_t items;
    /// @brief The type of heap, i.e. min-heap or max-heap.
    volatile uint32_t type;
    /// @brief The number of levels that the heap currently consists of.
    volatile uint32_t levels CACHE_ALIGN;
    /// @brief The last level used of the heap that is currently used.
    volatile uint32_t last_used_level;
    /// @brief The position with the largest index used in the last level.
    volatile uint32_t last_used_level_pos;
    /// @brief The size of heap's last level.
    volatile uint32_t last_used_level_size;
    /// @brief The i-th element of the array points to the i-th level of the heap.
    SynchHeapElement *heap_arrays[SYNCH_HEAP_MAX_LEVELS];
    /// @brief The array that stores the elements of the heap,
    SynchHeapElement *bulk;
} SerialHeapStruct;

/// @brief This function initializes an instance of the serial heap implementation.
/// This function should be called before any operation is applied to the heap.
inline void serialHeapInit(SerialHeapStruct *heap_state, uint32_t type);

/// @brief This function provides all the operations (i.e. insert an element, removing the minimum 
/// and getting the minimum) provided by this serial heap implementation.
///
/// @param state A pointer to an instance of the serial heap implementation.
/// @param arg In case that the user wants to perform:
/// (i) Getting the minimum element, arg should be equal to SYNCH_HEAP_GET_MIN_MAX_OP.
/// (ii) Removing the minimum element, arg should be equal to SYNCH_HEAP_DELETE_MIN_MAX_OP.
/// (iii) Inserting a new element ELMNT, arg should be equal to ELMNT | SYNCH_HEAP_INSERT_OP.
/// In case of inserting a new element ELMNT, the maximum value of ELMNT should be equal 
/// to SYNCH_HEAP_VAL_MASK.
/// @param pid The pid of the calling thread.
/// @return In case that the user wants to perform:
/// (i) Getting the minimum element; SYNCH_HEAP_EMPTY is returned in case that the heap is empty, 
/// otherwise the value of the minimum element is returned.
/// (ii) Removing the minimum element, SYNCH_HEAP_EMPTY is returned in case that the heap is empty, 
/// otherwise the value of the minimum element is returned.
/// (iii) Inserting a new element; SYNCH_HEAP_INSERT_FAIL is returned in case that there is 
/// no space available in the heap, otherwise SYNCH_HEAP_INSERT_SUCCESS nis returned.
inline RetVal serialHeapApplyOperation(void *state, ArgVal arg, int pid);

/// @brief This functions empties the heap from data by continuously executing HeapDeleteMinMax
/// operations. Moreover, if validates the ordering of the removed items.
/// @param heap_state A pointer to a heap object.
/// @return In case that the ordering of the removing items is valid, true is returned.
/// Otherwise, false is returned.
inline bool serialHeapClearAndValidation(SerialHeapStruct *heap_state);

#endif
