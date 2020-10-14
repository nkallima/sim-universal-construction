#include <primitives.h>

#ifdef NUMA_SUPPORT
#   include <numa.h>
#endif

#ifdef DEBUG
extern __thread int64_t __failed_cas;
extern __thread int64_t __executed_cas;
extern __thread int64_t __executed_swap;
extern __thread int64_t __executed_faa;
#endif

#ifdef __NO_GCC_SPARC__
inline static uint32_t __bitSearchFirst32(uint32_t v) {
    register uint32_t r;     // The result of log2(v) will be stored here
    register uint32_t shift;

    r =     (v > 0xFFFF) << 4; v >>= r;
    shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
    r |= (v >> 1);
    return r;
}

inline uint32_t bitSearchFirst(uint64_t v) {
    uint32_t r = __bitSearchFirst32((uint32_t)v);
    return (r == 0) ? __bitSearchFirst32((uint32_t)(v >> 32)) + 31 : r;
}
#endif


#ifdef __OLD_GCC_X86__
inline static bool __CASPTR(void *A, void *B, void *C) {
    uint64_t prev;
    uint64_t *p = (uint64_t *)A;

    asm volatile("lock;cmpxchgq %1,%2"
         : "=a"(prev)
         : "r"((uint64_t)C), "m"(*p), "0"((uint64_t)B)
         : "memory");
    return (prev == (uint64_t)B);
}

inline static bool __CAS64(volatile uint64_t *A, uint64_t B, uint64_t C) {
    uint64_t prev;
    uint64_t *p = (uint64_t *)A;

    asm volatile("lock;cmpxchgq %1,%2"
         : "=a"(prev)
         : "r"(C), "m"(*p), "0"(B)
         : "memory");
    return (prev == B);
}

inline static bool __CAS32(uint32_t *A, uint32_t B, uint32_t C) {
    uint32_t prev;
    uint32_t *p = (uint32_t *)A;

    asm volatile("lock;cmpxchgl %1,%2"
         : "=a"(prev)
         : "r"(C), "m"(*p), "0"(B)
         : "memory");
    return (prev == B);
}

inline static void *__SWAP(void *A, void *B) {
    int64_t *p = (int64_t *)A;

    asm volatile("lock;"
          "xchgq %0, %1"
          : "=r"(B), "=m"(*p)
          : "0"(B), "m"(*p)
          : "memory");
    return B;
}

inline static int64_t __FAA64(volatile int64_t *A, int64_t B){
    asm volatile("lock;"
          "xaddq %0, %1"
          : "=r"(B), "=m"(*A)
          : "0"(B), "m"(*A)
          : "memory");
    return B;
}

inline static int32_t __FAA32(volatile int32_t *A, int32_t B){
    asm volatile("lock;"
          "xaddl %0, %1"
          : "=r"(B), "=m"(*A)
          : "0"(B), "m"(*A)
          : "memory");
    return B;
}

inline int bitSearchFirst(uint64_t B) {
    uint64_t A;

    asm("bsfq %0, %1;" : "=d"(A) : "d"(B));

    return (int)A;
}

inline uint64_t nonZeroBits(uint64_t v) {
    uint64_t c;

    for (c = 0; v; v >>= 1)
        c += v & 1;

    return c;
}
#endif

inline void *getMemory(size_t size) {
    void *p;

#ifdef NUMA_SUPPORT
    p = numa_alloc_local(size);
#else
    p = malloc(size);
#endif
    if (p == null) {
        perror("memory allocation fail");
        exit(EXIT_FAILURE);
    } else return p;
}

inline void *getAlignedMemory(size_t align, size_t size) {
    void *p;

#ifdef NUMA_SUPPORT
    p = numa_alloc_local(size + align);
    long plong = (long)p;
    plong += align;
    plong &= ~(align - 1);
    p = (void *)plong;
#else
    p = (void *)memalign(align, size);
#endif

    if (p == null) {
        perror("memory allocation fail");
        exit(EXIT_FAILURE);
    } else return p;
}


inline void freeMemory(void *ptr, size_t size) {
#ifdef NUMA_SUPPORT
    numa_free(ptr, size);
#else
    free(ptr);
#endif
}

inline int64_t getTimeMillis(void) {
    struct timeb tm;

    ftime(&tm);
    return 1000 * tm.time + tm.millitm;
}

inline  bool _CAS32(uint32_t *A, uint32_t B, uint32_t C) {
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

inline void *_SWAP(void *A, void *B) {
#if defined(_EMULATE_SWAP_)
#    warning SWAP instructions are simulated!
    void *old_val;
    void *new_val;

    while (true) {
        old_val = (void *)*((volatile long *)A);
        new_val = B;
        if(((void *)*((volatile long *)A)) == old_val && CASPTR(A, old_val, new_val) == true)
            break;
    }
#   ifdef DEBUG
    __executed_swap++;
#   endif
    return old_val;

#else
#ifdef DEBUG
    __executed_swap++;
    return (void *)__SWAP(A, B);
#else
    return (void *)__SWAP(A, B);
#endif
#endif
}

inline int32_t _FAA32(volatile int32_t *A, int32_t B) {
#if defined(_EMULATE_FAA_)
#    warning Fetch&Add instructions are simulated!

    int32_t old_val;
    int32_t new_val;

    while (true) {
        old_val = *((int32_t * volatile)A);
        new_val = old_val + B;
        if(*A == old_val && CAS32(A, old_val, new_val) == true)
            break;
    }
#   ifdef DEBUG
        __executed_faa++;
#   endif
    return old_val;

#else
#   ifdef DEBUG
    __executed_faa++;
    return __FAA32(A, B);
#   else
    return __FAA32(A, B);
#   endif
#endif
}

inline int64_t _FAA64(volatile int64_t *A, int64_t B) {

#if defined(_EMULATE_FAA_)
#    warning Fetch&Add instructions are simulated!

    int64_t old_val;
    int64_t new_val;

    while (true) {
        old_val = *((int64_t * volatile)A);
        new_val = old_val + B;
        if(*A == old_val && CAS64(A, old_val, new_val) == true)
            break;
    }
#   ifdef DEBUG
        __executed_faa++;
#   endif
    return old_val;

#else
#   ifdef DEBUG
    __executed_faa++;
    return __FAA64(A, B);
#   else
    return __FAA64(A, B);
#   endif
#endif
}
