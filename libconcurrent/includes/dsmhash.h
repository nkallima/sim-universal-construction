/// @file dsmhash.h
/// @brief This file exposes the API of the DSM-Hash concurrent hash-table implementation based on the DSM-Synch combining technique [1].
///
/// This hash-table implementation uses an array of a fixed amount of cells that it is defined during the initialization of the data-structure. 
/// Each cell contains at most one <key,value> pair and a pointer to a next cell. For handling collisions this hash-table uses separate chaining.
/// For each cell there is a single instance of DSM-Synch. Whenever a thread wants to search, insert or delete a <key,value> pair in a
/// specific cell, it should use the corresponding DSM-Synch instance. Since this hash-table implementation is not expandable 
/// (the initial size of the array is fixed), it may become very slow in cases of storing big amounts of <key,value> pairs.
/// An example of use of this API is provided in benchmarks/dsmhashbench.c file.
///
/// References
/// ----------
/// [1]. Panagiota Fatourou, and Nikolaos D. Kallimanis. "Revisiting the combining synchronization technique". ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
#ifndef _DSMHASH_H_
#define _DSMHASH_H_

#include <stdint.h>
#include <primitives.h>
#include <dsmsynch.h>
#include <pool.h>
#include <types.h>

/// @brief A node (i.e. cell) for the linked-list of cells of <key,value> pair.
/// This should not directely accessed-used by the user.
typedef struct HashNode {
    /// @brief Pointer to next node (i.e. cell) of the linked list.
    struct HashNode *next;
    /// @brief The key of the <key,value> pair for the specific node.
    int64_t key;
    /// @brief The value of the <key,value> pair for the specific node.
    int64_t value;
} HashNode;

/// @brief HashOperations describes a hash-table operation, i.e. search, insert or delete.
/// This should not directely accessed-used by the user.
typedef struct HashOperations {
    /// @brief The key of the <key,value> pair for the specific request.
    int64_t key;
    /// @brief The value of the <key,value> pair for the specific request.
    int64_t value;
    /// @brief A new cell for the insert operations.
    HashNode *node;
    /// @brief This field stores result of the hash function.
    int32_t cell;
    /// @brief The type of hash-table operation, i.e. insert, delete or search.
    int32_t op;
} HashOperations;

/// @brief DSMHash stores the state of an instance of the a DSM-Hash concurrent hash-table.
/// DSMHash should be initialized using the DSMHashStructInit function.
typedef struct DSMHash {
    /// @brief A pointer to the announce array.
    HashOperations *announce;
    /// @brief A pointer to an array of DSM locks.
    DSMSynchStruct *synch CACHE_ALIGN;
    /// @brief A pointer to an array of the cells.
    ptr_aligned_t *cells;
    /// @brief The size in terms of cells of the hash-table.
    int size;
} DSMHash;

/// @brief DSMHashThreadState stores each thread's local state for a single instance of DSM-Hash.
/// For each instance of DSM-Hash, a discrete instance of DSMHashThreadState should be used.
typedef struct DSMHashThreadState {
    /// @brief A pointer to the thread's local state for the DSM-Synch instance.
    DSMSynchThreadState *th_state;
    /// @brief A pool of nodes for fast memory allocation.s
    SynchPoolStruct pool;
} DSMHashThreadState;

/// @brief This function initializes the DSM-Hash object, i.e. DSM-Hash struct. This function should be called once
/// (by a single thread) before any other thread tries to apply any request on the hash-table.
///
/// @param hash A pointer to the hash-table instance.
/// @param num_cells The number of cells that the hash-table object is going to use.
/// @param nthreads The number of threads that will use the DSM-Hash object.
inline void DSMHashInit(DSMHash *hash, int num_cells, int nthreads);

/// @brief This function should be called once before the thread applies any operation to the DSM-Hash combining object.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of DSM-Hash.
/// @param num_cells The number of cells that the hash-table object is going to use.
/// @param pid The pid of the calling thread.
inline void DSMHashThreadStateInit(DSMHash *hash, DSMHashThreadState *th_state, int num_cells, int pid);

/// @brief This function tries to insert a <key,value> into the hash-table if there is enough space in the
/// corresponding cell. If the key already exists in the hash-table, DSMHashInsert updates the corresponding value.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of DSM-Hash.
/// @param key The key of the <key,value> pair that DSMHashInsert will try to insert into the hash-table.
/// @param value The value of the <key,value> pair that DSMHashInsert will try to insert into the hash-table.
/// @param pid The pid of the calling thread.
/// @return In case of success true is returned. In case that there is not enough space in the corresponding cell.
inline bool DSMHashInsert(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int64_t value, int pid);

/// @brief This function searches for a specifc key in the hash-table. In case that DSMHashSearch finds the key,
/// it returns true. Otherwise, it returns false.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of DSM-Hash.
/// @param key The key of the <key,value> pair that DSMHashSearch will search for.
/// @param pid The pid of the calling thread.
/// @return DSMHashSearch returns the value of the <key,value> pair in case that the key exists in the hash-table.
inline RetVal DSMHashSearch(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid);

/// @brief This function searches for a specifc key in the hash-table. In case that DSMHashDelete finds the key,
/// it deletes the corresponding <key,value>.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of DSM-Hash.
/// @param key The key of the <key,value> pair that DSMHashDelete will try to delete it.
/// @param pid The pid of the calling thread.
inline void DSMHashDelete(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid);

#endif
