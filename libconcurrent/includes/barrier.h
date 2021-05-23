/// @file barrier.h
/// @brief This file exposes the API of a simple re-entrant synchronization barrier.
/// This implementation is a classic two phase re-entrant barrier, i.e. arrive and leave phases.
/// Examples of usage could be found in almost all the provided benchmarks under the benchmarks directory.
#ifndef _BARRIER_H_
#define _BARRIER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // @brief This integer is atomically increased by one whenever a thread enters the 
    volatile int32_t arrive;
    volatile int32_t leave;
    volatile bool arrive_flag;
    volatile bool leave_flag;
    volatile int32_t val_at_set;
} Barrier;

/// @brief This function initializes an instance of an re-entrant barrier object.
///
/// This function should be called once (by a single thread) before any other thread tries to any other function.
///
/// @param bar A pointer to an instance of the barrier object.
/// @param n The number of threads that will use the barrier object.
inline void BarrierSet(Barrier *bar, uint32_t n);

/// @brief Whenever a thread executes this function, it waits without returning 
/// until all the other n-1 threads also execute the same function.
/// @param bar A pointer to an instance of the barrier object.
inline void BarrierWait(Barrier *bar);

/// @brief Whenever a thread executes this function, it states that it will no use this barrier instance anymore. 
/// Thus, the system may free some resources.
/// @param bar A pointer to an instance of the barrier object.
inline void BarrierLeave(Barrier *bar);

/// @brief Deprecated functionality. It is only used in the threadtools.c file.
/// @param bar A pointer to an instance of the barrier object.
inline void BarrierLastLeave(Barrier *bar);

#endif