#include <primitives.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SYNCH_NUMA_SUPPORT
#    include <numa.h>
#endif

#define MAX_VENDOR_STR_SIZE 64

#ifdef DEBUG
extern __thread int64_t __failed_cas;
extern __thread int64_t __executed_cas;
extern __thread int64_t __executed_swap;
extern __thread int64_t __executed_faa;
#endif

#ifdef __OLD_GCC_X86__
inline bool __CASPTR(void *A, void *B, void *C) {
    uint64_t prev;
    uint64_t *p = (uint64_t *)A;

    asm volatile("lock;"
                 "cmpxchgq %1,%2" 
                 : "=a"(prev)
                 : "r"((uint64_t)C), "m"(*p), "0"((uint64_t)B) 
                 : "memory");
    return (prev == (uint64_t)B);
}

inline bool __CAS64(volatile uint64_t *A, uint64_t B, uint64_t C) {
    uint64_t prev;
    uint64_t *p = (uint64_t *)A;

    asm volatile("lock;"
                 "cmpxchgq %1,%2"
                 : "=a"(prev)
                 : "r"(C), "m"(*p), "0"(B)
                 : "memory");
    return (prev == B);
}

inline bool __CAS32(uint32_t *A, uint32_t B, uint32_t C) {
    uint32_t prev;
    uint32_t *p = (uint32_t *)A;

    asm volatile("lock;"
                 "cmpxchgl %1,%2" 
                 : "=a"(prev) 
                 : "r"(C), "m"(*p), "0"(B) 
                 : "memory");
    return (prev == B);
}

inline void *__SWAP(void *A, void *B) {
    int64_t *p = (int64_t *)A;

    asm volatile("lock;"
                 "xchgq %0, %1"
                 : "=r"(B), "=m"(*p)
                 : "0"(B), "m"(*p)
                 : "memory");
    return B;
}

inline int64_t __FAA64(volatile int64_t *A, int64_t B) {
    asm volatile("lock;"
                 "xaddq %0, %1"
                 : "=r"(B), "=m"(*A)
                 : "0"(B), "m"(*A)
                 : "memory");
    return B;
}

inline int32_t __FAA32(volatile int32_t *A, int32_t B) {
    asm volatile("lock;"
                 "xaddl %0, %1"
                 : "=r"(B), "=m"(*A)
                 : "0"(B), "m"(*A)
                 : "memory");
    return B;
}

inline uint64_t __BitTAS64(volatile uint64_t *A, unsigned char B) {
    int64_t *p = (int64_t *)A;
    int64_t bit = B;
    asm volatile("lock;" 
                 "btsq %0, %1"
                 : "=r"(bit), "=m"(*p)
                 : "0"(bit), "m"(*p)
                 : "memory");

    return bit;
}

inline int synchBitSearchFirst(uint64_t B) {
    uint64_t A;

    asm("bsfq %0, %1;" : "=d"(A) : "d"(B));

    return (int)A;
}

inline uint64_t synchNonZeroBits(uint64_t v) {
    uint64_t c;

    for (c = 0; v; v >>= 1)
        c += v & 1;

    return c;
}
#endif

inline bool _CAS128(uint64_t *A, uint64_t B0, uint64_t B1, uint64_t C0, uint64_t C1) {
    bool res;

#if defined(__OLD_GCC_X86__) || defined(__amd64__) || defined(__x86_64__)
    uint64_t dummy;

    asm volatile("lock;"
                 "cmpxchg16b %2; setz %1"
                 : "=d" (dummy), "=a" (res), "+m" (*A)
                 : "b" (C0), "c" (C1), "a" (B0),  "d" (B1));
#else
    __uint128_t old_value = (__uint128_t)(B0) | (((__uint128_t)(B1)) << 64ULL);
    __uint128_t new_value = (__uint128_t)(C0) | (((__uint128_t)(C1)) << 64ULL);
    res = __atomic_compare_exchange_16(A, &old_value, new_value, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif

#ifdef DEBUG
    __executed_cas++;
    __failed_cas += 1 - res;
#endif

    return res;
}

inline void *synchGetMemory(size_t size) {
    void *p;

#ifdef SYNCH_NUMA_SUPPORT
    p = numa_alloc_local(size);
#else
    p = malloc(size);
#endif
    if (p == NULL) {
        perror("memory allocation fail");
        exit(EXIT_FAILURE);
    } else
        return p;
}

inline void *synchGetAlignedMemory(size_t align, size_t size) {
    void *p;

#ifdef SYNCH_NUMA_SUPPORT
    p = numa_alloc_local(size + align);
    long plong = (long)p;
    plong += align;
    plong &= ~(align - 1);
    p = (void *)plong;
#else
    p = (void *)memalign(align, size);
#endif

    if (p == NULL) {
        perror("memory allocation fail");
        exit(EXIT_FAILURE);
    } else
        return p;
}

inline void synchFreeMemory(void *ptr, size_t size) {
#ifdef SYNCH_NUMA_SUPPORT
    numa_free(ptr, size);
#else
    free(ptr);
#endif
}

inline int64_t synchGetTimeMillis(void) {
    struct timespec tm;

    if (clock_gettime(CLOCK_MONOTONIC, &tm) == -1) {
        perror("clock_gettime");
        return 0;
    } else return tm.tv_sec*1000LL + tm.tv_nsec/1000000LL;
}

inline uint64_t synchGetMachineModel(void) {
#if defined(__amd64__) || defined(__x86_64__)
    char cpu_model[MAX_VENDOR_STR_SIZE] = {'\0'};

    asm volatile("movl $0, %%eax\n"
                 "cpuid\n"
                 "movl %%ebx, %0\n"
                 "movl %%edx, %1\n"
                 "movl %%ecx, %2\n"
                 : "=m"(cpu_model[0]), "=m"(cpu_model[4]), "=m"(cpu_model[8])
                 :: "%eax", "%ebx", "%edx", "%ecx", "memory");
#ifdef DEBUG
    fprintf(stderr, "DEBUG: Machine model: %s\n", cpu_model);
#endif

    if (strcmp(cpu_model, "AuthenticAMD") == 0)
        return AMD_X86_MACHINE;
    else if (strcmp(cpu_model, "GenuineIntel") == 0)
        return INTEL_X86_MACHINE;
    else
        return X86_GENERIC_MACHINE;
#elif defined(__aarch64__)
    return ARM_GENERIC_MACHINE;
#elif defined(__riscv__) || defined(__riscv)
    return RISCV_GENERIC_MACHINE;
#else
    return UNKNOWN_MACHINE;
#endif
}

inline bool _CASPTR(void *A, void *B, void *C) {
#ifdef DEBUG
    int res;

    res = __CASPTR(A, B, C);
    __executed_cas++;
    __failed_cas += 1 - res;

    return res;
#else
    return __CASPTR(A, B, C);
#endif
}

inline bool _CAS64(uint64_t *A, uint64_t B, uint64_t C) {
#ifdef DEBUG
    int res;

    res = __CAS64(A, B, C);
    __executed_cas++;
    __failed_cas += 1 - res;

    return res;
#else
    return __CAS64(A, B, C);
#endif
}

inline bool _CAS32(uint32_t *A, uint32_t B, uint32_t C) {
#ifdef DEBUG
    int res;

    res = __CAS32(A, B, C);
    __executed_cas++;
    __failed_cas += 1 - res;

    return res;
#else
    return __CAS32(A, B, C);
#endif
}

inline void *_SWAP(void *A, void *B) {
#if defined(_EMULATE_SWAP_)
#    warning synchSWAP instructions are simulated!
    void *old_val;
    void *new_val;

    while (true) {
        old_val = (void *)*((volatile long *)A);
        new_val = B;
        if (((void *)*((volatile long *)A)) == old_val && synchCASPTR(A, old_val, new_val) == true) break;
    }
#    ifdef DEBUG
    __executed_swap++;
#    endif
    return old_val;
#else
#    ifdef DEBUG
    __executed_swap++;
    return (void *)__SWAP(A, B);
#    else
    return (void *)__SWAP(A, B);
#    endif
#endif
}

inline int32_t _FAA32(volatile int32_t *A, int32_t B) {
#if defined(_EMULATE_FAA_)
#    warning Fetch&Add instructions are simulated!

    int32_t old_val;
    int32_t new_val;

    while (true) {
        old_val = *((int32_t *volatile)A);
        new_val = old_val + B;
        if (*A == old_val && synchCAS32(A, old_val, new_val) == true) break;
    }
#    ifdef DEBUG
    __executed_faa++;
#    endif
    return old_val;
#else
#    ifdef DEBUG
    __executed_faa++;
    return __FAA32(A, B);
#    else
    return __FAA32(A, B);
#    endif
#endif
}

inline int64_t _FAA64(volatile int64_t *A, int64_t B) {
#if defined(_EMULATE_FAA_)
#    warning Fetch&Add instructions are simulated!

    int64_t old_val;
    int64_t new_val;

    while (true) {
        old_val = *((int64_t *volatile)A);
        new_val = old_val + B;
        if (*A == old_val && synchCAS64(A, old_val, new_val) == true) break;
    }
#    ifdef DEBUG
    __executed_faa++;
#    endif
    return old_val;
#else
#    ifdef DEBUG
    __executed_faa++;
    return __FAA64(A, B);
#    else
    return __FAA64(A, B);
#    endif
#endif
}

inline uint64_t _BitTAS64(volatile uint64_t *A, unsigned char B) {
    return __BitTAS64(A, B);
}
