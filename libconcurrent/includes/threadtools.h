/// @file threadtools.h
/// @brief This file exposes a simple API for handling both posix and user-level threads.
/// This API provides functionality for creating new threads (both posix and user-level),
/// functionality for setting affinities, functionality for yielding the processor, etc.
/// Examples of usage could be found in almost all the provided benchmarks under the benchmarks directory.
#ifndef _THREAD_H_
#define _THREAD_H_

#include <config.h>
#include <stdint.h>
#include <stdbool.h>

#define SYNCH_DONT_USE_UTHREADS                       1
#define SYNCH_THREAD_PLACEMENT_FLAT                   0x1
#define SYNCH_THREAD_PLACEMENT_NUMA_SPARSE            0x2
#define SYNCH_THREAD_PLACEMENT_NUMA_DENSE             0x3
#define SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER 0x4
#define SYNCH_THREAD_PLACEMENT_NUMA_DENSE_SMT_PREFER  0x5
#define SYNCH_THREAD_PLACEMENT_DEFAULT                SYNCH_THREAD_PLACEMENT_NUMA_SPARSE

/// @brief This function creates nthreads posix threads, where each posix thread executes
/// uthreads user-level threads (fibers). Thus, the total amount of threads and fibers is nthreads * uthreads.
/// Each of the created threads executes the func function, the function has as an argument
/// the id of the created thread, which is a unique integer in {0, ..., nthreads * uthreads - 1}.
/// In case, the user does not want to create any fiber, uthreads should be equal to SYNCH_DONT_USE_UTHREADS.
/// @param nthreads The number of posix threads.
/// @param func A function that each fiber and posix thread should execute. This function has
/// as a single argument the unique id of the thread.
/// @param uthreads The number of fibers that each posix thread should execute.
int synchStartThreadsN(uint32_t nthreads, void *(*func)(void *), uint32_t uthreads);

/// @brief This function returns whenever all the created posix threads and fibers spawned by StartThreadsN
/// have completed the execution. 
/// @param nthreads The number of posix threads that StartThreadsN spawned.
void synchJoinThreadsN(uint32_t nthreads);

void synchSetThreadPlacementPolicy(uint32_t policy);

/// @brief This function sets the CPU affinity of the running thread to cpu_id, where cpu_id
/// should be a unique integer in {0, ..., N-1}, where N is the amount of available processing cores.
int synchThreadPin(int32_t cpu_id);

/// @brief This function returns the id of the running thread (posix or fiber). More specifically, it returns
/// a unique integer in {0, ..., N-1}, where N is the amount of the running threads. For example, if 3 Posix threads
/// are running, and 4 fiber threads are running inside each Posix thread, this function will return an integer
/// in the interval of {0, ...., 11}.
inline int32_t synchGetThreadId(void);

/// @brief This fuction returns the id of the current posix thread. 
/// This function should return an identical value for any fiber running in the same posix thread.
inline int32_t synchGetPosixThreadId(void);

/// @brief This function returns the core-id of the current posix thread or fiber. The core-id is a
/// unique integer in {0, ..., N-1}, where N is the amount of available processing cores.
inline int32_t synchGetPreferedCore(void);

/// @brief This function returns the core-id of the posix thread or fiber with id equal to pid. 
/// The core-id is a unique integer in {0, ..., N-1}, where N is the amount of available processing cores.
inline uint32_t synchPreferedCoreOfThread(uint32_t pid);

/// @brief This function returns the number of system's processing cores.
inline uint32_t synchGetNCores(void);

/// @brief In case that this function is called by a posix thread, it hints OS to give the CPU to
/// some other thread. In case that this function is called by a fiber, it gives the CPU control 
/// to the next fiber (if any) running in the same posix thread.
inline void synchResched(void);

/// @brief This function returns true if the number of spawned threads is greater than the number of 
/// system's available processing cores; otherwise, this function returns false.
inline bool synchIsSystemOversubscribed(void);

#endif
