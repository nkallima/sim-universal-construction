/// @file fastrand.h
/// @brief This file exposes a simple API for generating integer random numbers.
/// The main purpose of the provided random generators is to provide random numbers with low computational overhead.
/// Examples of usage could be found in almost all the provided benchmarks under the benchmarks directory.
#ifndef _FASTRAND_H_
#define _FASTRAND_H_

#include <stdint.h>

/// @brief The maximum number returned by synchFastRandom and fastRandomRange functions.
#define SYNCH_RAND_MAX 32768

/// @brief This random generators are implementing by following POSIX.1-2001 directives.
/// This function returns a positive integer in the range {0, ..., SYNCH_RAND_MAX}.
long synchFastRandom(void);

/// @brief A simple pseudo-random 32-bit number generator implementing the multiply-with-carry method
/// invented by George Marsaglia. It is computationally fast and has good properties
/// (see http://en.wikipedia.org/wiki/Random_number_generation#Computational_methods)
/// This function returns a positive 32-bit integer.
uint32_t synchFastRandom32(void);

/// @brief This function set a new seed to the random generator.
/// @param seed The new seed for the random generator.
void synchFastRandomSetSeed(uint32_t seed);

/// @brief This random generators are implementing by following POSIX.1-2001 directives.
/// This function returns a positive integer in the range {low, ..., high}.
/// Notice that high should be less or equal to SYNCH_RAND_MAX; otherwise the behavior is undefined.
/// Moreover, low should be less than high; otherwise the behavior is undefined.
long synchFastRandomRange(long low, long high);

/// @brief A simple pseudo-random 32-bit number generator implementing the multiply-with-carry method
/// invented by George Marsaglia. It is computationally fast and has good properties
/// (see http://en.wikipedia.org/wiki/Random_number_generation#Computational_methods)
/// This function returns a positive 32-bit integer in the range {low, ..., high}.
uint32_t synchFastRandomRange32(uint32_t low, uint32_t high);

#endif
