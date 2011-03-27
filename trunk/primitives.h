#ifndef _PRIMITIVES_H_
#define _PRIMITIVES_H_

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <math.h>
#include <stdint.h>
#include <sys/timeb.h>
#include <malloc.h>

//#define _EMULATE_FAA_

#ifndef CACHE_LINE_SIZE
#    define CACHE_LINE_SIZE            64
#endif


#define CACHE_ALIGN                    __attribute__ ((aligned (CACHE_LINE_SIZE)))
#define BIG_ALIGN                      __attribute__ ((aligned (8)))
#define PAD_CACHE(A)                  ((CACHE_LINE_SIZE - (A % CACHE_LINE_SIZE))/sizeof(int32_t))


#ifndef USE_CPUS
#    if defined(linux)
#        define USE_CPUS               sysconf(_SC_NPROCESSORS_ONLN)
#    else
#        define USE_CPUS               1
#    endif
#endif


#define null                           NULL
#define bool                           int_fast32_t
#define true                           1
#define false                          0


inline static void *getMemory(size_t size) {
    void *p = malloc(size);

    if (p == null) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    } else return p;
}

inline static void *getAlignedMemory(size_t align, size_t size) {
    void *p;
	
    p = (void *)memalign(align, size);
    if (p == null) {
        perror("memalign");
        exit(EXIT_FAILURE);
    } else return p;
}

inline static int64_t getTimeMillis(void) {
	struct timeb tm;

	ftime(&tm);
	return 1000 * tm.time + tm.millitm;
}


#if defined(__GNUC__) && (__GNUC__*10000 + __GNUC_MINOR__*100) >= 40100
#    define CAS(A, B, C)               __sync_bool_compare_and_swap(A, B, C)
#    define GAS(A, B)                  __sync_lock_test_and_set(A, B)
#    define FAA(A, B)                  __sync_fetch_and_add(A, B)
#    define ReadPrefetch(A)            __builtin_prefetch((const void *)A, 0, 3);
#    define StorePrefetch(A)           __builtin_prefetch((const void *)A, 1, 3);
#    define bitSearchFirst(A, B)       A = __builtin_ctzll(B)
#    define oneSetBits(A)              __builtin_popcountll(A)

#    if defined(__amd64__) || defined(__x86_64__)
#        define LoadFence()            asm volatile ("lfence":::"memory")
#        define StoreFence()           asm volatile ("sfence":::"memory")
#        define FullFence()            asm volatile ("mfence":::"memory")

inline static unsigned long long RDTSCLL(void)
{
  unsigned hi, lo;

  asm volatile ("rdtsc":"=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#    else
#        define LoadFence()            __sync_synchronize()
#        define StoreFence()           __sync_synchronize()
#        define FullFence()            __sync_synchronize()
#    endif

#elif defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
#    warning A newer GCC version is recommended!
#    define CAS(A, B, C)               __CAS(A, (int64_t)B, (int64_t)C)
#    define FAA(A, B)                  asm("lock; xaddq %0,%1": "+r" (B), "+m" (*(A)): : "memory")
#    define GAS(A, B)                  __GAS(A, (int64_t)B)
#    define bitSearchFirst(A, B)       asm("bsfq %0, %1;" : "=d"(A) : "d"(B))
#    define LoadFence()                asm volatile ("lfence":::"memory") 
#    define StoreFence()               asm volatile ("sfence":::"memory") 
#    define FullFence()                asm volatile ("mfence":::"memory") 
#    define ReadPrefetch(A)            asm volatile ("prefetch0 %0"::"m"(A))
#    define StorePrefetch(A)           asm volatile ("prefetch0 %0"::"m"(A))

inline static int64_t __GAS(volatile void *A, int64_t B) {
    int64_t *p = (int64_t *)A;

    asm volatile("lock;"
          "xchgq %0, %1"
          : "=r"(B), "=m"(*p)
          : "0"(B), "m"(*p)
          : "memory");
    return B;
}

inline static bool __CAS(volatile void *A, int64_t B, int64_t C) {
    int64_t prev;
    int64_t *p = (int64_t *)A;

    asm volatile("lock;cmpxchgq %1,%2"
         : "=a"(prev)
         : "r"(C), "m"(*p), "0"(B)
         : "memory");
    return (prev == B);
}

inline static uint64_t oneSetBits(uint64_t v) {
    uint64_t c;

    for (c = 0; v; v >>= 1)
        c += v & 1;

    return c;
}

#elif defined(sun) && defined(sparc) && defined(__SUNPRO_C)
#    warning Experimental support!

#    include <atomic.h>
#    include <sun_prefetch.h>

     extern void MEMBAR_ALL(void);
     extern void MEMBAR_STORE(void);
     extern void MEMBAR_LOAD(void);
     extern void *CASPO(void volatile*, void *);
     extern void *SWAPPO(void volatile*, void *);
     extern int32_t POPC(int32_t x);
	 extern void NOP(void);

#    define CAS(A, B, C)               (atomic_cas_ptr(A, (void *)B, (void *)C) == (void *)B)
#    define GAS(A, B)                  SWAPPO(A, B)
#    define FAA(A, B)                  atomic_add_ptr(A, B)
#    define oneSetBits(A)              (POPC((int32_t)A)+POPC((int32_t)(A>>32)))
#    define bitSearchFirst(A, B)       A = __bitSearchFirst(B)
#    define LoadFence()                MEMBAR_LOAD()
#    define StoreFence()               MEMBAR_STORE()
#    define FullFence()                MEMBAR_ALL()
#    define ReadPrefetch(A)            sparc_prefetch_read_many((void *)A)
#    define StorePrefetch(A)           sparc_prefetch_write_many((void *)A)

inline static uint32_t __bitSearchFirst32(uint32_t v) {
    register uint32_t r;     // result of log2(v) will go here
    register uint32_t shift;

    r =     (v > 0xFFFF) << 4; v >>= r;
    shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
    r |= (v >> 1);
    return r;
}

inline static uint32_t __bitSearchFirst(uint64_t v) {
    uint32_t r = __bitSearchFirst32((uint32_t)v);
	return (r == 0) ? __bitSearchFirst32((uint32_t)(v >> 32)) + 31 : r;
}

#else
#    error Current machine architecture and compiler are not supported yet!
#endif


#if defined(_EMULATE_FAA_)
#    undef FAA
#    undef __FAA
#    define FAA(A, B)                  __FAA((int64_t * volatile)A, B)
     inline long long __FAA(void *ptr, int64_t v) {
        int64_t old_val;
        int64_t new_val;

        while (true) {
             old_val = *((int64_t * volatile)ptr);
             new_val = old_val + v;
             if(CAS((int64_t * volatile)ptr, old_val, new_val) == true)
                 break;
        }
        return old_val;
     }
#endif

#endif
