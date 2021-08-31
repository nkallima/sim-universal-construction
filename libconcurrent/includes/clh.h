/// @file clh.h
/// @brief This file exposes the API of the CLH queue locks.
/// An example of use of this API is provided in benchmarks/clhbench.c file.
///
/// For a more detailed description see the following two publications:
/// (1) Travis S. Craig. "Building FIFO and priority-queueing spin locks from atomic swap". Technical Report TR 93-02-02, Department of Computer Science, University of Washington, February 1993.
/// (2) Peter Magnusson, Anders Landin, and Erik Hagersten. "Queue locks on cache coherent multiprocessors". Parallel Processing Symposium, 1994. Proceedings., Eighth International. IEEE, 1994.
/// @copyright Copyright (c) 2021
#ifndef _CLH_H_
#define _CLH_H_

#include <primitives.h>
#include <stdbool.h>

/// @brief CLHLockStruct used for announcing that a thread wants to acquire the lock.
typedef union CLHLockNode {
    /// @brief This is true whenever a thread executes CLHLock and waits until it acquires the lock.
    bool locked;
    /// @brief Padding space.
    char align[CACHE_LINE_SIZE];
} CLHLockNode;

/// @brief CLHLockStruct stores the state of an instance of the CLH queue lock implementation.
/// CLHLockStruct should be initialized using the CLHLockInit function.
typedef struct CLHLockStruct {
    /// @brief Pointer to a list of threads that want to enter to the critical section.
    volatile CLHLockNode *Tail CACHE_ALIGN;
    /// @brief An array of nthreads pointers to CLHLockNode structs (one struct per thread).
    volatile CLHLockNode **MyNode CACHE_ALIGN;
    /// @brief An array of nthreads pointers to CLHLockNode structs (one struct per thread).
    /// Initially, each such pointer points to NULL.
    volatile CLHLockNode **MyPred;
} CLHLockStruct;

/// @brief This function initializes an instance of the CLH queue lock implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// use either CLHLock or CLHUnlock.
///
/// @param nthreads The number of threads that will use the CLH queue lock implementation.
CLHLockStruct *CLHLockInit(uint32_t nthreads);

/// @brief This function returns if the lock is available and the calling thread successfully acquires it.
/// In case that the lock is already locked by another thread, the calling thread shall block until the lock becomes available (unlocked)
/// and the calling thread successfully acquires it.
/// 
/// @param l A pointer to an instance of the CLH queue lock.
/// @param pid The pid of the calling thread.
void CLHLock(CLHLockStruct *l, int pid);

/// @brief This function makes the lock available (unlocked).
/// @param l A pointer to an instance of the CLH queue lock.
/// @param pid The pid of the calling thread.
void CLHUnlock(CLHLockStruct *l, int pid);

#endif
