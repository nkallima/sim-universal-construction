#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <config.h>

#ifndef S_CACHE_LINE_SIZE
#    define S_CACHE_LINE_SIZE 256
#endif

// For Intel Xeon multiprocessors, it seems that choosing sizes of cache-line greater
// or equal to 128 bytes give much better performance.
// In contrast to Intel machines, Amd multiprocessors behave well with cache-line sizes
// of 64 bytes. Thus, a safe choice id 128. In any case, perform some experiments
// in order to find out the best value for cache-line size.
#ifdef SYNCH_COMPACT_ALLOCATION
#    define CACHE_LINE_SIZE 64
#else
#    define CACHE_LINE_SIZE 512
#endif

#ifdef __GNUC__
#    define S_CACHE_ALIGN __attribute__((aligned(S_CACHE_LINE_SIZE)))
#    define CACHE_ALIGN   __attribute__((aligned(CACHE_LINE_SIZE)))
#    define VAR_ALIGN     __attribute__((aligned(16)))
#else
#    define S_CACHE_ALIGN
#    define CACHE_ALIGN
#    define VAR_ALIGN
#endif

#define PAD_CACHE(A) ((CACHE_LINE_SIZE - (A % CACHE_LINE_SIZE)) / sizeof(char))

#endif
