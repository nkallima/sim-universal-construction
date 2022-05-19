#include <fastrand.h>
#include <limits.h>
#include <math.h>

static __thread long __fast_random_next = 1;
static __thread uint32_t __fast_random_next_z = 2;
static __thread uint32_t __fast_random_next_w = 2;

long synchFastRandom(void) {
    __fast_random_next = __fast_random_next * 1103515245 + 12345;
    return ((unsigned)(__fast_random_next / 65536) % 32768);
}

// A simple pseudo-random 32-bit number generator implementing the multiply-with-carry method
// invented by George Marsaglia. It is computationally fast and has good properties.
// http://en.wikipedia.org/wiki/Random_number_generation#Computational_methods
uint32_t synchFastRandom32(void) {
    __fast_random_next_z = 36969 * (__fast_random_next_z & 65535) + (__fast_random_next_z >> 16);
    __fast_random_next_w = 18000 * (__fast_random_next_w & 65535) + (__fast_random_next_w >> 16);
    return (__fast_random_next_z << 16) + __fast_random_next_w; /* 32-bit result */
}

void synchFastRandomSetSeed(uint32_t seed) {
    __fast_random_next = (long)seed;
    __fast_random_next_z = seed;
    __fast_random_next_w = seed / 2;

    if (__fast_random_next_z == 0 || __fast_random_next_z == 0x9068ffff)
        __fast_random_next_z++;
    if (__fast_random_next_w == 0 || __fast_random_next_w == 0x464fffff)
        __fast_random_next_w++;
}

// In Numerical Recipes in C: The Art of Scientific Computing
// (William H. Press, Brian P. Flannery, Saul A. Teukolsky, William T. Vetterling;
// New York: Cambridge University Press, 1992 (2nd ed., p. 277))
// -------------------------------------------------------------------------------
uint32_t synchFastRandomRange32(uint32_t low, uint32_t high) {
    return low + (uint32_t)(((double)high - low) * ((double)synchFastRandom32() / (UINT_MAX)));
}

long synchFastRandomRange(long low, long high) {
    return low + (long)(((double)high - low) * ((double)synchFastRandom() / (SYNCH_RAND_MAX + 1.0)));
}