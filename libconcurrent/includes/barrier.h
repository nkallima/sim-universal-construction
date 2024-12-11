/// @file barrier.h
/// @brief This file exposes the API of a simple re-entrant synchronization barrier.
/// This implementation is a classic two phase (arrive and leave phases) re-entrant barrier, i.e. arrive and leave phases.
/// Examples of usage could be found in almost all the provided benchmarks under the benchmarks directory.
#ifndef _BARRIER_H_
#define _BARRIER_H_

#include <stdint.h>
#include <stdbool.h>

/// @brief Barrier stores the state of an instance of the a backoff object.
/// Barrier should be initialized using the BarrierSet function.
typedef struct SynchBarrier {
    /// @brief This integer is atomically decreased by one whenever a thread enters the arrive phase of the barrier.
    volatile int32_t arrive;
    /// @brief This integer is atomically decreased by one whenever a thread enters the leave phase of the barrier.
    volatile int32_t leave;
    /// @brief This variable is set to true if all the n threads have entered to the arrive phase.
    volatile bool arrive_flag;
    /// @brief This variable is set to true if all the n threads have entered to the leave phase.
    volatile bool leave_flag;
    /// @brief The initial value set to arrive and leave variables.
    volatile int32_t val_at_set;
} SynchBarrier;

/// @brief This function initializes an instance of an re-entrant barrier object.
///
/// This function should be called once (by a single thread) before any other thread tries to any other function.
///
/// @param bar A pointer to an instance of the barrier object.
/// @param n The number of threads that will use the barrier object.
void synchBarrierSet(SynchBarrier *bar, uint32_t n);

/// @brief Whenever a thread executes this function, it waits without returning 
/// until all the other n-1 threads also execute the same function.
/// @param bar A pointer to an instance of the barrier object.
void synchBarrierWait(SynchBarrier *bar);

/// @brief Whenever a thread executes this function, it states that it will no use this barrier instance anymore. 
/// Thus, the system may free some resources.
/// @param bar A pointer to an instance of the barrier object.
void synchBarrierLeave(SynchBarrier *bar);

/// @brief Deprecated functionality. It is only used in the threadtools.c file.
/// @param bar A pointer to an instance of the barrier object.
void synchBarrierLastLeave(SynchBarrier *bar);

#endif