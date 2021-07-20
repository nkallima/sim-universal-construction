/// @file mcs.h
/// @brief This file exposes the API of the MCS queue locks.
/// An example of use of this API is provided in benchmarks/mcsbench.c file.
///
/// For a more detailed description see the following publication:
/// John M. Mellor-Crummey, and Michael L. Scott. "Algorithms for scalable synchronization on shared-memory multiprocessors". ACM Transactions on Computer Systems (TOCS) 9.1 (1991): 21-65.
#ifndef _MCS_H_
#define _MCS_H_

#include <config.h>
#include <primitives.h>
#include <stdint.h>
#include <stdbool.h>

/// @brief HalfMCSLockNode used for appropriately padding MCSLockNode.
typedef struct HalfMCSLockNode {
    volatile bool locked;
    volatile struct MCSLockNode *next;
} HalfMCSLockNode;

/// @brief MCSLockNode used for announcing that a thread wants to acquire the lock.
typedef struct MCSLockNode {
    /// @brief This is true whenever a thread executes MCSLock and waits until it acquires the lock.
    volatile bool locked;
    /// @brief Pointer to the next thread that wants to enter to the critical section.
    volatile struct MCSLockNode *next;
    /// @brief Padding space.
    char pad[PAD_CACHE(sizeof(HalfMCSLockNode))];
} MCSLockNode;

/// @brief MCSLockStruct stores the state of an instance of the MCS queue lock implementation.
/// MCSLockStruct should be initialized using the MCSLockInit function.
typedef struct MCSLockStruct {
    /// @brief Pointer to a list of threads that want to enter to the critical section.
    volatile MCSLockNode *Tail CACHE_ALIGN;
} MCSLockStruct;

/// @brief MCSThreadState stores the state of a thread's local instance of the MCS queue lock implementation.
/// MCSThreadState should be initialized using the MCSThreadStateInit function.
typedef struct MCSThreadState {
    volatile MCSLockNode *MyNode CACHE_ALIGN;
} MCSThreadState;


/// @brief This function initializes an instance of the MCS queue lock implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// use either MCSLock or MCSUnlock.
MCSLockStruct *MCSLockInit(void);

/// @brief This function should be called once before the thread applies any operation to the MCS queue lock.
///
/// @param st_thread A pointer to thread's local state of the MCS queue lock.
/// @param pid The pid of the calling thread.
void MCSThreadStateInit(MCSThreadState *st_thread, int pid);

/// @brief This function returns if the lock is available and the calling thread successfully acquires it.
/// In case that the lock is already locked by another thread, the calling thread shall block until the lock becomes available (unlocked)
/// and the calling thread successfully acquires it.
/// 
/// @param l A pointer to an instance of the MCS queue lock.
/// @param thread_state A pointer to thread's local state of the MCS queue lock.
/// @param pid The pid of the calling thread.
void MCSLock(MCSLockStruct *l, MCSThreadState *thread_state, int pid);

/// @brief This function makes the lock available (unlocked).
/// 
/// @param l A pointer to an instance of the MCS queue lock.
/// @param thread_state A pointer to thread's local state of the MCS queue lock.
/// @param pid The pid of the calling thread.
void MCSUnlock(MCSLockStruct *l, MCSThreadState *thread_state, int pid);
#endif
