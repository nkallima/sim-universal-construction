#ifndef _FASTRAND_H_
#define _FASTRAND_H_

#include <stdint.h>

#define SIM_RAND_MAX 32768

// This random generators are implementing
// by following POSIX.1-2001 directives.
// ---------------------------------------

long fastRandom(void);
uint32_t fastRandom32(void);
void fastRandomSetSeed(uint32_t seed);
long fastRandomRange(long low, long high);
uint32_t fastRandomRange32(uint32_t low, uint32_t high);

#endif
