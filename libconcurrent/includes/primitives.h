/// @file primitives.h
/// @brief This file exposes a simple API for basic atomic operations, basic operations for memory management.
/// Moreover, this file provides functionality for finding out the the vendor of the processor and some 
/// basic functionality for measuring time.
#ifndef _PRIMITIVES_H_
#define _PRIMITIVES_H_

#include <stdint.h>
#include <stdbool.h>
#include <config.h>
#include <system.h>
#include <stats.h>
#include <stddef.h>

/// @brief The vendor of the processor is unknown.
#define UNKNOWN_MACHINE       0x0
/// @brief The vendor is an AMD x86 processor.
#define AMD_X86_MACHINE       0x1
/// @brief The vendor is an Intel X86 processor.
#define INTEL_X86_MACHINE     0x2
/// @brief This is a generic X86 processor.
#define X86_GENERIC_MACHINE   0x3
/// @brief This is a generic ARM processor.
#define ARM_GENERIC_MACHINE   0x4
/// @brief This is a generic RISC-V processor.
#define RISCV_GENERIC_MACHINE 0x5

#if defined(__GNUC__) && (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40100
#    define __CAS128(A, B0, B1, C0, C1) _CAS128(A, B0, B1, C0, C1)
#    define __CASPTR(A, B, C)           __sync_bool_compare_and_swap((long *)A, (long)B, (long)C)
#    define __CAS64(A, B, C)            __sync_bool_compare_and_swap(A, B, C)
#    define __CAS32(A, B, C)            __sync_bool_compare_and_swap(A, B, C)
#    define __SWAP(A, B)                __sync_lock_test_and_set((long *)A, (long)B)
#    define __FAA64(A, B)               __sync_fetch_and_add(A, B)
#    define __FAA32(A, B)               __sync_fetch_and_add(A, B)
#    define __BitTAS64(A, B)            __sync_fetch_and_or(A, (1ULL << (B)))
#    define synchReadPrefetch(A)        __builtin_prefetch((const void *)A, 0, 3);
#    define synchStorePrefetch(A)       __builtin_prefetch((const void *)A, 1, 3);
#    define synchBitSearchFirst(A)      __builtin_ctzll(A)
#    define synchNonZeroBits(A)         __builtin_popcountll(A)
#    define synchLikely(A)              __builtin_expect(!!(A), 1)
#    define synchUnlikely(A)            __builtin_expect(!!(A), 0)
#    define UNUSED_ARG                  __attribute__((unused))
#    if defined(__amd64__) || defined(__x86_64__)
#        define synchLoadFence()  asm volatile("lfence" ::: "memory")
#        define synchStoreFence() asm volatile("sfence" ::: "memory")
#        define synchFullFence()  asm volatile("mfence" ::: "memory")
#        define synchNonTSOFence()
#    else
#        define synchLoadFence()   __sync_synchronize()
#        define synchStoreFence()  __sync_synchronize()
#        define synchFullFence()   __sync_synchronize()
#        define synchNonTSOFence() __sync_synchronize()
#    endif
#elif defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
#    warning You may lose performance!
#    warning A newer version of GCC compiler is recommended!
#    define synchLoadFence()      asm volatile("lfence" ::: "memory")
#    define synchStoreFence()     asm volatile("sfence" ::: "memory")
#    define synchFullFence()      asm volatile("mfence" ::: "memory")
#    define synchReadPrefetch(A)  asm volatile("prefetchnta %0" ::"m"(*((const int *)A)))
#    define synchStorePrefetch(A) asm volatile("prefetchnta %0" ::"m"(*((const int *)A)))
#    define synchNonTSOFence()
#    define synchLikely(A)   (A)
#    define synchUnlikely(A) (A)
#    define UNUSED_ARG       __attribute__((unused))
//   in this case where gcc is too old, implement atomic primitives in primitives.c
#    define __OLD_GCC_X86__
inline int synchBitSearchFirst(uint64_t B);
inline uint64_t synchNonZeroBits(uint64_t v);
#else
#    error Current machine architecture and compiler are not supported yet!
#endif

#if defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
/// @brief This macro emits a X86 pause instruction.
#    define synchPause()                                                                                                                                                                               \
        {                                                                                                                                                                                              \
            int __i;                                                                                                                                                                                   \
            for (__i = 0; __i < 16; __i++) {                                                                                                                                                           \
                asm volatile("pause");                                                                                                                                                                 \
                asm volatile("pause");                                                                                                                                                                 \
                asm volatile("pause");                                                                                                                                                                 \
                asm volatile("pause");                                                                                                                                                                 \
            }                                                                                                                                                                                          \
        }
#else
/// @brief This macro emits a NO-OP instruction.
#    define synchPause()
#endif

/// @brief This function allocates a memory area of size bytes.
/// In case that NUMA_SUPPORT is defined in libconcurrent/config.h, the returned memory is allocated on the local NUMA node.
///
/// @param size The size of the memory area.
/// @return In case of error, NULL is returned. In case of success a pointer to the allocated memory area is returned.
inline void *synchGetMemory(size_t size);

/// @brief This function allocates a memory area of size bytes. The returned address is aligned to an offset equal to align bytes. 
/// In case that NUMA_SUPPORT is defined in libconcurrent/config.h, the returned memory is allocated on the local NUMA node.
///
/// @param align The alignment size.
/// @param size The size of the memory area.
/// @return In case of error, NULL is returned. In case of success a pointer to the allocated memory area is returned.
inline void *synchGetAlignedMemory(size_t align, size_t size);

/// @brief This function frees memory allocated with either getMemory() or getAlignedMemory() functions.
///
/// @param ptr A pointer to the memory area to be freed.
/// @param size The size of the memory area to be freed.
inline void synchFreeMemory(void *ptr, size_t size);

/// @brief This function returns the current system's time in milliseconds.
///
/// @return System's time in milliseconds.
inline int64_t synchGetTimeMillis(void);

/// @brief This function returns the vendor of the processor that it runs on.
/// The current version of the Synch framework returns any of the following codes:
/// - AMD_X86_MACHINE
/// - INTEL_X86_MACHINE
/// - X86_GENERIC_MACHINE
/// - ARM_GENERIC_MACHINE
/// - RISCV_GENERIC_MACHINE
/// - UNKNOWN_MACHINE
///
/// @return It returns a code for the vendor of the processor that it runs on.
inline uint64_t synchGetMachineModel(void);

/// A wrapper for the _CAS128 function. See more on _CAS128().
#define synchCAS128(A, B0, B1, C0, C1) _CAS128((uint64_t *)(A), (uint64_t)(B0), (uint64_t)(B1), (uint64_t)(C0), (uint64_t)(C1))
/// @brief This function is executed atomically. It reads the 128-bit value stored at a memory location pointed by A. 
/// Let A0 be the less significant 64-bits of this memory location and let A1 the most significant.
/// This function computes '(A0 == B0 & A1 == B1) ? <C0,C1> : <A0,A1>' and stores the result at location pointed by A.
/// The function returns in case that (A0 == B0 & A1 == B1) is true. Otherwise, it returns false.
///
/// @param A A pointer to 128-bit value stored to memory.
/// @param B0 The less significant 64-bits of the old value.
/// @param B1 The most significant 64-bits of the old value.
/// @param C0 The less significant 64-bits of the new value.
/// @param C1 The most significant 64-bits of the new value.
/// @return This function computes '(A0 == B0 & A1 == B1) ? <C0,C1> : <A0,A1>' and stores the result at location pointed by A.
/// The function returns in case that (A0 == B0 & A1 == B1) is true. Otherwise, it returns false.
inline bool _CAS128(uint64_t *A, uint64_t B0, uint64_t B1, uint64_t C0, uint64_t C1);

/// A wrapper for the _CASPTR function. See more on _CASPTR().
#define synchCASPTR(A, B, C) _CASPTR((void *)(A), (void *)(B), (void *)(C))
/// @brief This function is executed atomically. It reads the pointer stored at a memory location pointed by A. 
/// Let OLD be the pointer of the memory location pointed by A. This function computes '(OLD == B) ? C : OLD' and stores the result
/// at location pointed by A. The function returns in case that (OLD == B) is true. Otherwise, it returns false.
///
/// @param A A pointer to memory location that stores a memory pointer.
/// @param B The old value.
/// @param C The new value.
/// @return The function returns in case that (OLD == B) is true. Otherwise, it returns false.
inline bool _CASPTR(void *A, void *B, void *C);

/// A wrapper for the _CAS64 function. See more on _CAS64().
#define synchCAS64(A, B, C) _CAS64((uint64_t *)(A), (uint64_t)(B), (uint64_t)(C))
/// @brief This function is executed atomically. It reads the 64-bit value stored at a memory location pointed by A. 
/// Let OLD be the value of the memory location pointed by A. This function computes '(OLD == B) ? C : OLD' and stores the result
/// at location pointed by A. The function returns in case that (OLD == B) is true. Otherwise, it returns false.
///
/// @param A A pointer to 64-bit value stored to memory.
/// @param B The old value (64-bits).
/// @param C The new value (64-bits).
/// @return The function returns in case that (OLD == B) is true. Otherwise, it returns false.
inline bool _CAS64(uint64_t *A, uint64_t B, uint64_t C);

/// A wrapper for the _CAS32 function. See more on _CAS32().
#define synchCAS32(A, B, C) _CAS32((uint32_t *)(A), (uint32_t)(B), (uint32_t)(C))
/// @brief This function is executed atomically. It reads the 32-bit value stored at a memory location pointed by A. 
/// Let OLD be the value of the memory location pointed by A. This function computes '(OLD == B) ? C : OLD' and stores the result
/// at location pointed by A. The function returns in case that (OLD == B) is true. Otherwise, it returns false.
///
/// @param A A pointer to 32-bit value stored to memory.
/// @param B The old value (32-bits).
/// @param C The new value (32-bits).
/// @return The function returns in case that (OLD == B) is true. Otherwise, it returns false.
inline bool _CAS32(uint32_t *A, uint32_t B, uint32_t C);

/// A wrapper for the _SWAP function. See more on _SWAP().
#define synchSWAP(A, B) _SWAP((void *)(A), (void *)(B))
/// @brief This function is executed atomically. It performs an atomic exchange operation on the memory location pointed by A,
/// setting B as the new value. It returns the old value of the memory location pointed by A just before the operation.
///
/// @param A A pointer to memory location that stores a memory pointer.
/// @param B The new value to be stored in the memory location pointed by A.
/// @return It returns the old value of the memory location pointed by A just before the operation. 
inline void *_SWAP(void *A, void *B);

/// A wrapper for the _FAA32 function. See more on _FAA32().
#define synchFAA32(A, B) _FAA32((volatile int32_t *)(A), (int32_t)(B))
/// @brief This function is executed atomically. It performs an atomic addition of value B to the value pointed by A.
/// It returns the old value of the memory location pointed by A just before the operation.
///
/// @param A A pointer to memory location that stores a 32-bit integer.
/// @param B A 32-bit integer to be added to the value pointed by A.
/// @return It returns the old 32-bit value of the memory location pointed by A just before the operation.
inline int32_t _FAA32(volatile int32_t *A, int32_t B);

/// A wrapper for the _FAA64 function. See more on _FAA64().
#define synchFAA64(A, B) _FAA64((volatile int64_t *)(A), (int64_t)(B))
/// @brief This function is executed atomically. It performs an atomic addition of value B to the value pointed by A.
/// It returns the old value of the memory location pointed by A just before the operation.
///
/// @param A A pointer to memory location that stores a 64-bit integer.
/// @param B A 64-bit integer to be added to the value pointed by A.
/// @return It returns the old 64-bit value of the memory location pointed by A just before the operation.
inline int64_t _FAA64(volatile int64_t *A, int64_t B);

/// A wrapper for the _BitTAS64 function. See more on _BitTAS64().
#define synchBitTAS64(A, B) _BitTAS64((volatile uint64_t *)(A), (unsigned char)(B))
/// @brief This function is executed atomically. It performs an atomic Test&Set on the B-th bit of the value pointed by A.
///
/// @param A A pointer to memory location that stores a 64-bit value.
/// @param B The B-th bit of the value stored in the memory location pointed by A.
/// @return The result of Test&Set on the B-th bit of the value pointed by A.
inline uint64_t _BitTAS64(volatile uint64_t *A, unsigned char B);

#endif
