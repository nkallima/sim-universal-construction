#ifndef _PRIMITIVES_H_
#define _PRIMITIVES_H_

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <math.h>
#include <stdint.h>
#include <sys/timeb.h>
#include <malloc.h>

#include <config.h>
#include <system.h>
#include <types.h>
#include <threadtools.h>
#include <stats.h>


#if defined(__GNUC__) && (__GNUC__*10000 + __GNUC_MINOR__*100) >= 40100
#    define __CASPTR(A, B, C)          __sync_bool_compare_and_swap((long *)A, (long)B, (long)C)
#    define __CAS64(A, B, C)           __sync_bool_compare_and_swap(A, B, C)
#    define __CAS32(A, B, C)           __sync_bool_compare_and_swap(A, B, C)
#    define __SWAP(A, B)               __sync_lock_test_and_set((long *)A, (long)B)
#    define __FAA64(A, B)              __sync_fetch_and_add(A, B)
#    define __FAA32(A, B)              __sync_fetch_and_add(A, B)
#    define ReadPrefetch(A)            __builtin_prefetch((const void *)A, 0, 3);
#    define StorePrefetch(A)           __builtin_prefetch((const void *)A, 1, 3);
#    define bitSearchFirst(A)          __builtin_ctzll(A)
#    define nonZeroBits(A)             __builtin_popcountll(A)
#    if defined(__amd64__) || defined(__x86_64__)
#        define LoadFence()            asm volatile ("lfence":::"memory")
#        define StoreFence()           asm volatile ("sfence":::"memory")
#        define FullFence()            asm volatile ("mfence":::"memory")
#    else
#        define LoadFence()            __sync_synchronize()
#        define StoreFence()           __sync_synchronize()
#        define FullFence()            __sync_synchronize()
#    endif

#elif defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
#    warning A newer version of GCC compiler is recommended!
#    define LoadFence()                asm volatile ("lfence":::"memory") 
#    define StoreFence()               asm volatile ("sfence":::"memory") 
#    define FullFence()                asm volatile ("mfence":::"memory") 
#    define ReadPrefetch(A)            asm volatile ("prefetchnta %0"::"m"(*((const int *)A)))
#    define StorePrefetch(A)           asm volatile ("prefetchnta %0"::"m"(*((const int *)A)))

//   in this case where gcc is too old, implement atomic primitives in primitives.c
#    define                            __OLD_GCC_X86__
inline int bitSearchFirst(uint64_t B);
inline uint64_t nonZeroBits(uint64_t v);

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

#    define __CASPTR(A, B, C)          (atomic_cas_ptr(A, B, C) == B)
#    define __CAS32(A, B, C)           (atomic_cas_32(A, B, C) == B)
#    define __CAS64(A, B, C)           (atomic_cas_64(A, B, C) == B)
#    define __SWAP(A, B)               SWAPPO(A, B)
#    define __FAA32(A, B)              atomic_add_32_nv((volatile uint32_t *)A, (int32_t)B)
#    define __FAA64(A, B)              atomic_add_64_nv((volatile uint64_t *)A, (int64_t)B)
#    define nonZeroBits(A)             (POPC((int32_t)A)+POPC((int32_t)(A>>32)))
#    define LoadFence()                MEMBAR_LOAD()
#    define StoreFence()               MEMBAR_STORE()
#    define FullFence()                MEMBAR_ALL()
#    define ReadPrefetch(A)            sparc_prefetch_read_many((void *)A)
#    define StorePrefetch(A)           sparc_prefetch_write_many((void *)A)

#    define                            __NO_GCC_SPARC__
inline uint32_t bitSearchFirst(uint64_t v) ;

#else
#    error Current machine architecture and compiler are not supported yet!
#endif



#if defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
#    define Pause()                    {int __i; for (__i = 0; __i < 16; __i++) {\
                                                     asm volatile ("pause");\
                                                     asm volatile ("pause");\
                                                     asm volatile ("pause");\
                                                     asm volatile ("pause");}}

#elif defined(sparc)
#    define Pause()                    FullFence()
#else
#    define Pause()                      
#endif

inline void *getMemory(size_t size);
inline void *getAlignedMemory(size_t align, size_t size);
inline void freeMemory(void *ptr, size_t size);

inline int64_t getTimeMillis(void);

#define CAS32(A, B, C) _CAS32((uint32_t *)A, (uint32_t)B, (uint32_t)C)
inline  bool _CAS32(uint32_t *A, uint32_t B, uint32_t C);

#define CAS64(A, B, C) _CAS64((uint64_t *)A, (uint64_t)B, (uint64_t)C)
inline bool _CAS64(uint64_t *A, uint64_t B, uint64_t C);

#define CASPTR(A, B, C) _CASPTR((void *)A, (void *)B, (void *)C)
inline bool _CASPTR(void *A, void *B, void *C);

#define SWAP(A, B) _SWAP((void *)A, (void *)B)
inline void *_SWAP(void *A, void *B);

#define FAA32(A, B) _FAA32((volatile int32_t *)A, (int32_t)B)
inline int32_t _FAA32(volatile int32_t *A, int32_t B);

#define FAA64(A, B) _FAA64((volatile int64_t *)A, (int64_t)B);
inline int64_t _FAA64(volatile int64_t *A, int64_t B);

#endif
