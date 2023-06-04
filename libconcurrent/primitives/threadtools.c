#define _GNU_SOURCE
#include <unistd.h>

#include <config.h>
#include <threadtools.h>
#include <primitives.h>
#include <uthreads.h>
#include <barrier.h>
#include <sched.h> // CPU_SET, CPU_ZERO, cpu_set_t, sched_setaffinity()
#include <pthread.h>
#include <stdio.h>

#ifdef SYNCH_NUMA_SUPPORT
#    include <numa.h>
#endif

inline static void *uthreadWrapper(void *arg);
inline static void *kthreadWrapper(void *arg);

static __thread pthread_t *__threads;
static __thread int32_t __thread_id = 0;
static __thread int32_t __prefered_core = 0;
static __thread int32_t __unjoined_threads = 0;

static void *(*__func)(void *) CACHE_ALIGN = NULL;
static uint32_t __uthreads = 0;
static uint32_t __nthreads = 0;
static uint32_t __ncores = 0;
static uint32_t __schecule_policy = SYNCH_THREAD_PLACEMENT_DEFAULT;
static bool __uthread_sched = false;
static bool __system_oversubscription = false;
static bool __noop_resched = false;
static SynchBarrier bar CACHE_ALIGN;

void synchSetThreadPlacementPolicy(uint32_t policy) {
    __schecule_policy = policy;
    synchFullFence();
}

void setThreadId(int32_t id) {
    __thread_id = id;
}

inline int32_t synchGetPreferedCore(void) {
    return __prefered_core;
}

inline uint32_t synchGetNCores(void) {
    if (__ncores == 0)
        __ncores = sysconf(_SC_NPROCESSORS_ONLN);
    return __ncores;
}

inline static void *kthreadWrapper(void *arg) {
    int cpu_id;
    long pid = (long)arg;

    cpu_id = pid % synchGetNCores();
    synchThreadPin(cpu_id);
    setThreadId(pid);
    synchStartCPUCounters(pid);
    __func((void *)pid);
    synchStopCPUCounters(pid);
    synchBarrierLeave(&bar);
    return NULL;
}

inline uint32_t synchPreferedCoreOfThread(uint32_t pid) {
    uint32_t prefered_core = 0;

    if (__schecule_policy == SYNCH_THREAD_PLACEMENT_FLAT) {
        prefered_core = pid;
    } else {
#ifdef SYNCH_NUMA_SUPPORT
        int ncpus = numa_num_configured_cpus();
        int nodes = numa_num_task_nodes();
        int node_size = ncpus / nodes;

        if (__schecule_policy == SYNCH_THREAD_PLACEMENT_NUMA_SPARSE) {
            if (numa_node_of_cpu(0) == numa_node_of_cpu(ncpus / 2)) { // SMT or HyperThreading detected
                int half_node_size = node_size / 2;
                int offset = 0;
                uint32_t half_cpu_id = pid;

                if (pid >= ncpus / 2) {
                    half_cpu_id = pid - ncpus / 2;
                    offset = ncpus / 2;
                }
                prefered_core = (half_cpu_id % nodes) * half_node_size + half_cpu_id / nodes;
                prefered_core += offset;
            } else prefered_core = ((pid % nodes) * node_size);
        } else if (__schecule_policy == SYNCH_THREAD_PLACEMENT_NUMA_SPARSE_SMT_PREFER) {
            if (numa_node_of_cpu(0) == numa_node_of_cpu(ncpus / 2)) { // SMT or HyperThreading detected
                uint32_t double_nodes = 2 * nodes;
                uint32_t half_node_size = node_size / 2;

                prefered_core = (pid % node_size) * half_node_size + (pid / double_nodes);
            } else prefered_core = ((pid % nodes) * node_size);
        } else if (__schecule_policy == SYNCH_THREAD_PLACEMENT_NUMA_DENSE) {
            prefered_core = pid;
        } else if (__schecule_policy == SYNCH_THREAD_PLACEMENT_NUMA_DENSE_SMT_PREFER){
            prefered_core = (pid / nodes) + (pid % nodes) * (node_size);
        } else {
            fprintf(stderr, "ERROR: Unsupported scheduling policy: 0x%X\n", __schecule_policy);
            prefered_core = ((pid % nodes) * node_size);
        }
#else
        prefered_core = pid;
#endif
    }
    prefered_core %= synchGetNCores();

    return prefered_core;
}

int synchThreadPin(int32_t cpu_id) {
    int ret = 0;
    cpu_set_t mask;
    unsigned int len = sizeof(mask);

    pthread_setconcurrency(synchGetNCores());
    CPU_ZERO(&mask);
    __prefered_core = synchPreferedCoreOfThread(cpu_id);
    CPU_SET(__prefered_core, &mask);
#if defined(DEBUG) && defined(SYNCH_NUMA_SUPPORT)
    fprintf(stderr, "DEBUG: posix_thread: %d -- numa_node: %d -- core: %d\n", cpu_id, numa_node_of_cpu(__prefered_core), __prefered_core);
#endif
    ret = sched_setaffinity(0, len, &mask);
    if (ret == -1)
        perror("sched_setaffinity");

    return ret;
}

inline static void *uthreadWrapper(void *arg) {
    int i, kernel_id;
    long pid = (long)arg;

    kernel_id = (pid / __uthreads) % synchGetNCores();
    synchThreadPin(kernel_id);
    setThreadId(pid);
    synchStartCPUCounters(kernel_id);
    synchInitFibers(__uthreads);
    for (i = 0; i < __uthreads - 1; i++) {
        synchSpawnFiber(__func, pid + i + 1);
#if defined(DEBUG)
        fprintf(stderr, "DEBUG: fiber: %ld\n", pid + i + 1);
#endif
    }
#if defined(DEBUG)
    fprintf(stderr, "DEBUG: fiber: %ld\n", pid);
#endif
    __func((void *)pid);

    synchWaitForAllFibers();
    synchStopCPUCounters(kernel_id);
    synchBarrierLeave(&bar);
    return NULL;
}

int synchStartThreadsN(uint32_t nthreads, void *(*func)(void *), uint32_t uthreads) {
    long i;
    int last_thread_id = -1;

    synchInitCPUCounters();
    __ncores = sysconf(_SC_NPROCESSORS_ONLN);
    __nthreads = nthreads;
    __threads = synchGetMemory(nthreads * sizeof(pthread_t));
    __func = func;
    synchStoreFence();
    if (uthreads != SYNCH_DONT_USE_UTHREADS && uthreads > 1) {
        __uthreads = uthreads;
        __uthread_sched = true;
        __system_oversubscription = true;
        synchBarrierSet(&bar, nthreads / uthreads + 1);
        for (i = 0; i < (nthreads / uthreads) - 1; i++) {
            last_thread_id = pthread_create(&__threads[i], NULL, uthreadWrapper, (void *)(i * uthreads));
            if (last_thread_id != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
            __unjoined_threads++;
        }
        uthreadWrapper((void *)(i * uthreads));
    } else {
        __uthread_sched = false;
        if (__nthreads > __ncores)
            __system_oversubscription = true;
        else
            __noop_resched = true;
        synchBarrierSet(&bar, nthreads + 1);
        for (i = 0; i < nthreads - 1; i++) {
            last_thread_id = pthread_create(&__threads[i], NULL, kthreadWrapper, (void *)i);
            if (last_thread_id != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
            __unjoined_threads++;
        }
        kthreadWrapper((void *)i);
    }
    return last_thread_id;
}

void synchJoinThreadsN(uint32_t nthreads) {
    synchBarrierLastLeave(&bar);
    synchFreeMemory(__threads, nthreads * sizeof(pthread_t));
}

inline int32_t synchGetThreadId(void) {
    return __thread_id + synchCurrentFiberIndex();
}

inline int32_t synchGetPosixThreadId(void) {
    return __thread_id;
}

inline void synchResched(void) {
    if (__noop_resched) {
        synchPause();
    } else if (__uthread_sched) {
        synchFiberYield();
    } else {
        sched_yield();
    }
}

inline bool synchIsSystemOversubscribed(void) {
    return __system_oversubscription;
}
