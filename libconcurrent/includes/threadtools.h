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

/// @brief Threads are distributed in a round-robin fashion across all processing cores.
#define SYNCH_THREAD_PLACEMENT_FLAT                   0x1

/// @brief It optimizes thread placement for systems with Non-Uniform Memory Access (NUMA) by spreading threads sparsely 
/// across NUMA nodes, potentially improving memory bandwidth and improving cache utilization.
#define SYNCH_THREAD_PLACEMENT_NUMA_SPARSE            0x2

/// @brief It places threads within the smallest number of NUMA nodes before spreading them to other nodes,
/// which can improve memory locality and may reduce contention on shared variables.
#define SYNCH_THREAD_PLACEMENT_NUMA_DENSE             0x3

/// @brief Similar to `SYNCH_THREAD_PLACEMENT_NUMA_DENSE`, but with a preference for utilizing Simultaneous Multithreading (SMT)
/// capabilities within NUMA nodes to maximize processing efficiency.
#define SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER 0x4

/// @brief It combines the sparse distribution strategy across NUMA nodes with a preference for SMT.
/// This policy spreads threads across NUMA nodes to avoid contention, while preferring to fill SMT slots within each core before moving to
/// the next. It aims to strike a balance between improving memory bandwidth and leveraging SMT for higher processing efficiency and reduced
/// contention on shared variables.
#define SYNCH_THREAD_PLACEMENT_NUMA_DENSE_SMT_PREFER  0x5

/// @brief By default the thread placement policy is se to `SYNCH_THREAD_PLACEMENT_DEFAULT`.
/// Currently, `SYNCH_THREAD_PLACEMENT_DEFAULT` is equal to `SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER`.
#define SYNCH_THREAD_PLACEMENT_DEFAULT                SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER

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

///@brief This function sets the default placement policy of threads in machine's processors. 
/// The thread placement policy could set any of the following:
/// - `SYNCH_THREAD_PLACEMENT_FLAT`: Threads are distributed in a round-robin fashion across all processing cores.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_SPARSE`: Optimizes thread placement for systems with Non-Uniform Memory Access (NUMA)
/// by spreading threads sparsely across NUMA nodes, potentially improving memory bandwidth and improving cache utilization.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_DENSE`: Places threads within the smallest number of NUMA nodes before spreading them to other nodes,
/// which can improve memory locality and may reduce contention on shared variables.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_DENSE_SMT_PREFER`: Similar to `SYNCH_THREAD_PLACEMENT_NUMA_DENSE`, but with a preference 
/// for utilizing Simultaneous Multithreading (SMT) capabilities within NUMA nodes to maximize processing efficiency.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER`: Combines the sparse distribution strategy across NUMA nodes with a preference for SMT.
/// This policy spreads threads across NUMA nodes to avoid contention, while preferring to fill SMT slots within each core before moving to
/// the next. It aims to strike a balance between improving memory bandwidth and leveraging SMT for higher processing efficiency and reduced
/// contention on shared variables.
/// - `SYNCH_THREAD_PLACEMENT_DEFAULT`: By default the thread placement policy is se to `SYNCH_THREAD_PLACEMENT_DEFAULT`.
/// Currently, `SYNCH_THREAD_PLACEMENT_DEFAULT` is equal to `SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER`.
/// @param policy The thread placement policy to be set.
void synchSetThreadPlacementPolicy(uint32_t policy);

/// @brief Retrieves the current thread placement policy for the machine's processors.
/// This function returns the policy setting that determines how threads are distributed across the processing cores of the machine.
/// The possible return values correspond to the thread placement policies that could be returned are the following:
/// - `SYNCH_THREAD_PLACEMENT_FLAT`: Threads are distributed in a round-robin fashion across all processing cores.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_SPARSE`: Optimizes thread placement for systems with Non-Uniform Memory Access (NUMA)
/// by spreading threads sparsely across NUMA nodes, potentially improving memory bandwidth and improving cache utilization.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_DENSE`: Places threads within the smallest number of NUMA nodes before spreading them to other nodes,
/// which can improve memory locality and may reduce contention on shared variables.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_DENSE_SMT_PREFER`: Similar to `SYNCH_THREAD_PLACEMENT_NUMA_DENSE`, but with a preference 
/// for utilizing Simultaneous Multithreading (SMT) capabilities within NUMA nodes to maximize processing efficiency.
/// - `SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER`: Combines the sparse distribution strategy across NUMA nodes with a preference for SMT.
/// This policy spreads threads across NUMA nodes to avoid contention, while preferring to fill SMT slots within each core before moving to
/// the next. It aims to strike a balance between improving memory bandwidth and leveraging SMT for higher processing efficiency and reduced
/// contention on shared variables.
/// - `SYNCH_THREAD_PLACEMENT_DEFAULT`: By default the thread placement policy is se to `SYNCH_THREAD_PLACEMENT_DEFAULT`.
/// Currently, `SYNCH_THREAD_PLACEMENT_DEFAULT` is equal to `SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER`.
uint32_t synchGetThreadPlacementPolicy(void);

/// @brief This function sets the CPU affinity of the running thread to cpu_id, where cpu_id
/// should be a unique integer in {0, ..., N-1}, where N is the amount of available processing cores.
int synchThreadPin(int32_t cpu_id);

inline uint32_t synchPreferredNumaNodeOfThread(uint32_t pid);

/// @brief This function returns the id of the running thread (posix or fiber). More specifically, it returns
/// a unique integer in {0, ..., N-1}, where N is the amount of the running threads. For example, if 3 Posix threads
/// are running, and 4 fiber threads are running inside each Posix thread, this function will return an integer
/// in the interval of {0, ...., 11}.
inline int32_t synchGetThreadId(void);

inline int32_t synchGetPreferredNumaNode(void);

/// @brief This fuction returns the id of the current posix thread. 
/// This function should return an identical value for any fiber running in the same posix thread.
inline int32_t synchGetPosixThreadId(void);

/// @brief This function returns the core-id of the current posix thread or fiber. The core-id is a
/// unique integer in {0, ..., N-1}, where N is the amount of available processing cores.
inline int32_t synchGetPreferredCore(void);

/// @brief This function returns the core-id of the posix thread or fiber with id equal to pid. 
/// The core-id is a unique integer in {0, ..., N-1}, where N is the amount of available processing cores.
inline uint32_t synchPreferredCoreOfThread(uint32_t pid);

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
