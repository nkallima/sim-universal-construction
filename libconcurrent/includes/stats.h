/// @file stats.h
/// @brief This file exposes the API for keeping statistics in the provided data-structures.
/// The provided statistics are the total number of atomic instructions, the amount of each 
/// type of atomic instructions, atomic instructions per operation, etc.
/// Notice that this API should be used only by Posix threads. In the case where there more 
/// than one fiber per Posix thread, only a single fiber thread should use this API.
/// In case that the API of threadtools.h is used, most the provided functionality 
/// (except printStats function), should not be directly used by the user.
#ifndef _STATS_H_
#define _STATS_H_

#include <stdint.h>

/// @brief This function initiates the counters for keeping statics. 
/// This function should be called once, usually at the beginning of a main function.
/// In case that the API of threadtools.h is used, there is no need to directly use this function.
void synchInitCPUCounters(void);

/// @brief This function starts the logging of statistics (usually called before performing the first concurrent operation)
/// for the current thread with pid equal to id. In case that the API of threadtools.h is used, there is no need 
/// to directely use this function.
void synchStartCPUCounters(int id);

/// @brief This function stops the logging of statistics (usually called after performing the last concurrent operation)
/// for the current thread with pid equal to id. In case that the API of threadtools.h is used, there is no need to directly
/// use this function.
void synchStopCPUCounters(int id);

/// @brief This function prints statistics for all the running threads. This function should be called once after all running
/// threads threads have called the synchStopCPUCounters function. A good place for calling this function is to place as a last
/// instruction just before the return of main function (examples of usage could be found in almost all the provided benchmarks
/// under the benchmarks directory.).
/// @param nthreads The total number of threads that have executed concurrent operations.
/// @param runs The total number of the executed operations. Notice that benchmarks for stacks and queues
/// execute SYNCH_RUNS pairs of operations (i.e. pairs of push/pops or pairs of enqueues/dequeues).
void synchPrintStats(uint32_t nthreads, uint64_t runs);

#endif
