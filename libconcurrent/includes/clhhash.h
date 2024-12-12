/// @file clhhash.h
/// @brief This file exposes the API of the CLH-Hash concurrent hash-table implementation based on the CLH queue locks [1, 2].
///
/// This hash-table implementation uses an array of a fixed amount of cells that it is defined during the initialization of the data-structure. 
/// Each cell contains at most one <key,value> pair and a pointer to a next cell. For handling collisions this hash-table uses separate chaining.
/// Each cell of the array is protected by a single CLH lock. Whenever a thread wants to search, insert or delete a <key,value> pair in a
/// specific cell, it should first acquire the lock of the cell. Since this hash-table implementation is not expandable 
/// (the initial size of the array is fixed), it may become very slow in cases of storing big amounts of <key,value> pairs.
/// An example of use of this API is provided in benchmarks/clhhashbench.c file.
///
/// References
/// ----------
/// [1]. Travis S. Craig. "Building FIFO and priority-queueing spin locks from atomic swap". Technical Report TR 93-02-02, Department of Computer Science, University of Washington, February 1993.
/// [2]. Peter Magnusson, Anders Landin, and Erik Hagersten. "Queue locks on cache coherent multiprocessors". Parallel Processing Symposium, 1994. Proceedings., Eighth International. IEEE, 1994.
#ifndef _CLHHASH_H_
#define _CLHHASH_H_

#include <stdint.h>
#include <primitives.h>
#include <types.h>
#include <pool.h>
#include <clh.h>

/// @brief A node (i.e. cell) for the linked-list of cells of <key,value> pair.
/// This should not directly accessed-used by the user.
typedef struct HashNode {
    /// @brief Pointer to next node (i.e. cell) of the linked list.
    struct HashNode *next;
    /// @brief The key of the <key,value> pair for the specific node.
    int64_t key;
    /// @brief The value of the <key,value> pair for the specific node.
    int64_t value;
} HashNode;

/// @brief HashOperations describes a hash-table operation, i.e. search, insert or delete.
/// This should not directly accessed-used by the user.
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

/// @brief CLHHash stores the state of an instance of the a CLH-Hash concurrent hash-table.
/// CLHHash should be initialized using the CLHHashStructInit function.
typedef struct CLHHash {
    /// @brief A pointer to the announce array.
    HashOperations *announce;
    /// @brief A pointer to an array of CLH locks.
    CLHLockStruct **synch CACHE_ALIGN;
    /// @brief A pointer to an array of the cells.
    ptr_aligned_t *cells;
    /// @brief The size in terms of cells of the hash-table.
    int size;
} CLHHash;

/// @brief CLHHashThreadState stores each thread's local state for a single instance of CLH-Hash.
/// For each instance of CLH-Hash, a discrete instance of CLHHashThreadState should be used.
typedef struct CLHHashThreadState {
    /// @brief A pool of nodes for fast memory allocation.
    SynchPoolStruct pool;
} CLHHashThreadState;


/// @brief This function initializes the CLH-Hash object, i.e. CLH-Hash struct. This function should be called once
/// (by a single thread) before any other thread tries to apply any request on the hash-table.
///
/// @param hash A pointer to the hash-table instance.
/// @param num_cells The number of cells that the hash-table object is going to use.
/// @param nthreads The number of threads that will use the CLH-Hash object.
void CLHHashStructInit(CLHHash *hash, int num_cells, int nthreads);

/// @brief This function should be called once before the thread applies any operation to the CLH-Hash combining object.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of CLH-Hash.
/// @param num_cells The number of cells that the hash-table object is going to use.
/// @param pid The pid of the calling thread.
void CLHHashThreadStateInit(CLHHash *hash, CLHHashThreadState *th_state, int num_cells, int pid);

/// @brief This function tries to insert a <key,value> into the hash-table if there is enough space in the
/// corresponding cell. If the key already exists in the hash-table, CLHHashInsert updates the corresponding value.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of CLH-Hash.
/// @param key The key of the <key,value> pair that CLHHashInsert will try to insert into the hash-table.
/// @param value The value of the <key,value> pair that CLHHashInsert will try to insert into the hash-table.
/// @param pid The pid of the calling thread.
/// @return In case of success true is returned. In case that there is not enough space in the corresponding cell.
bool CLHHashInsert(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int64_t value, int pid);

/// @brief This function searches for a specific key in the hash-table. In case that CLHHashSearch finds the key,
/// it returns true. Otherwise, it returns false.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of CLH-Hash.
/// @param key The key of the <key,value> pair that CLHHashSearch will search for.
/// @param pid The pid of the calling thread.
/// @return CLHHashSearch returns the value of the <key,value> pair in case that the key exists in the hash-table.
RetVal CLHHashSearch(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid);

/// @brief This function searches for a specific key in the hash-table. In case that CLHHashDelete finds the key,
/// it deletes the corresponding <key,value>.
///
/// @param hash A pointer to the hash-table instance.
/// @param th_state A pointer to thread's local state of CLH-Hash.
/// @param key The key of the <key,value> pair that CLHHashDelete will try to delete it.
/// @param pid The pid of the calling thread.
void CLHHashDelete(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid);

#endif
