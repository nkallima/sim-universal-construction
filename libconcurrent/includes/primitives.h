#ifndef _PRIMITIVES_H_
#define _PRIMITIVES_H_

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#include <config.h>
#include <system.h>
#include <types.h>
#include <threadtools.h>
#include <stats.h>

#if defined(__GNUC__) && (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40100
#    define __CAS128(A, B0, B1, C0, C1) _CAS128(A, B0, B1, C0, C1)
#    define __CASPTR(A, B, C)           __sync_bool_compare_and_swap((long *)A, (long)B, (long)C)
#    define __CAS64(A, B, C)            __sync_bool_compare_and_swap(A, B, C)
#    define __CAS32(A, B, C)            __sync_bool_compare_and_swap(A, B, C)
#    define __SWAP(A, B)                __sync_lock_test_and_set((long *)A, (long)B)
#    define __FAA64(A, B)               __sync_fetch_and_add(A, B)
#    define __FAA32(A, B)               __sync_fetch_and_add(A, B)
#    define __BitTAS64(A, B)            __sync_fetch_and_or(A, (1ULL << (B)))
#    define ReadPrefetch(A)             __builtin_prefetch((const void *)A, 0, 3);
#    define StorePrefetch(A)            __builtin_prefetch((const void *)A, 1, 3);
#    define bitSearchFirst(A)           __builtin_ctzll(A)
#    define nonZeroBits(A)              __builtin_popcountll(A)
#    define Likely(A)                   __builtin_expect(!!(A), 1)
#    define Unlikely(A)                 __builtin_expect(!!(A), 0)
#    define UNUSED_ARG                  __attribute__((unused))
#    if defined(__amd64__) || defined(__x86_64__)
#        define LoadFence()   asm volatile("lfence" ::: "memory")
#        define StoreFence()  asm volatile("sfence" ::: "memory")
#        define FullFence()   asm volatile("mfence" ::: "memory")
#        define NonTSOFence()
#    else
#        define LoadFence()   __sync_synchronize()
#        define StoreFence()  __sync_synchronize()
#        define FullFence()   __sync_synchronize()
#        define NonTSOFence() __sync_synchronize()
#    endif
#elif defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
#    warning You may lose performance!
#    warning A newer version of GCC compiler is recommended!
#    define LoadFence()      asm volatile("lfence" ::: "memory")
#    define StoreFence()     asm volatile("sfence" ::: "memory")
#    define FullFence()      asm volatile("mfence" ::: "memory")
#    define ReadPrefetch(A)  asm volatile("prefetchnta %0" ::"m"(*((const int *)A)))
#    define StorePrefetch(A) asm volatile("prefetchnta %0" ::"m"(*((const int *)A)))
#    define NonTSOFence()
#    define Likely(A)        (A)
#    define Unlikely(A)      (A)
#    define UNUSED_ARG       __attribute__((unused))
//   in this case where gcc is too old, implement atomic primitives in primitives.c
#    define __OLD_GCC_X86__
inline int bitSearchFirst(uint64_t B);
inline uint64_t nonZeroBits(uint64_t v);
#else
#    error Current machine architecture and compiler are not supported yet!
#endif

#if defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
#    define Pause()                         \
        {                                   \
            int __i;                        \
            for (__i = 0; __i < 16; __i++) {\
                asm volatile("pause");      \
                asm volatile("pause");      \
                asm volatile("pause");      \
                asm volatile("pause");      \
            }                               \
        }
#else
#    define Pause()
#endif

inline void *getMemory(size_t size);
inline void *getAlignedMemory(size_t align, size_t size);
inline void freeMemory(void *ptr, size_t size);
inline int64_t getTimeMillis(void);

#define CAS128(A, B0, B1, C0, C1) _CAS128((uint64_t *)(A), (uint64_t)(B0), (uint64_t)(B1), (uint64_t)(C0), (uint64_t)(C1))
inline bool _CAS128(uint64_t *A, uint64_t B0, uint64_t B1, uint64_t C0, uint64_t C1);

#define CASPTR(A, B, C) _CASPTR((void *)(A), (void *)(B), (void *)(C))
inline bool _CASPTR(void *A, void *B, void *C);

#define CAS64(A, B, C) _CAS64((uint64_t *)(A), (uint64_t)(B), (uint64_t)(C))
inline bool _CAS64(uint64_t *A, uint64_t B, uint64_t C);

#define CAS32(A, B, C) _CAS32((uint32_t *)(A), (uint32_t)(B), (uint32_t)(C))
inline bool _CAS32(uint32_t *A, uint32_t B, uint32_t C);

#define SWAP(A, B) _SWAP((void *)(A), (void *)(B))
inline void *_SWAP(void *A, void *B);

#define FAA32(A, B) _FAA32((volatile int32_t *)(A), (int32_t)(B))
inline int32_t _FAA32(volatile int32_t *A, int32_t B);

#define FAA64(A, B) _FAA64((volatile int64_t *)(A), (int64_t)(B))
inline int64_t _FAA64(volatile int64_t *A, int64_t B);

#define BitTAS64(A, B) _BitTAS64((volatile uint64_t *)(A), (unsigned char)(B))
inline uint64_t _BitTAS64(volatile uint64_t *A, unsigned char B);

#endif
