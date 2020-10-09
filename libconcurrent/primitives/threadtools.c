#include <config.h>
#include <threadtools.h>
#include <primitives.h>
#include <uthreads.h>
#include <barrier.h>

#ifdef NUMA_SUPPORT
#   include <numa.h>
#endif

inline static void *uthreadWrapper(void *arg);
inline static void *kthreadWrapper(void *arg);

static __thread pthread_t *__threads;
static __thread int32_t __thread_id = -1;
static __thread int32_t __prefered_core = -1;
static __thread int32_t __unjoined_threads = 0;

static void *(*__func)(void *) CACHE_ALIGN = null;
static uint32_t __uthreads = 0;
static uint32_t __nthreads = 0;
static bool __uthread_sched CACHE_ALIGN = false;
static Barrier bar CACHE_ALIGN;

void setThreadId(int32_t id) {
    __thread_id = id;
}

int32_t getPreferedCore(void) {
    return __prefered_core;
}

inline static void *kthreadWrapper(void *arg) {
    int cpu_id;
    long pid = (long)arg;

    cpu_id = pid % USE_CPUS;
    threadPin(cpu_id);
    setThreadId(pid);
    start_cpu_counters(pid);
    __func((void *)pid);
    stop_cpu_counters(pid);
    BarrierLeave(&bar);
    return null;
}

int threadPin(int32_t cpu_id) {
    pthread_setconcurrency(USE_CPUS);

#if defined(__sun) || defined(sun)
    int ret;

    ret = processor_bind(P_LWPID, cpu_id + 1, (cpu_id + 1)%USE_CPUS, null);
    if (ret == -1)
        perror("processor_bind");
        
    return ret;
    
#elif defined (__gnu_linux__) || defined (__gnu__linux) || defined (__linux__)
    int ret = 0;
    cpu_set_t mask;  
    unsigned int len = sizeof(mask);

    CPU_ZERO(&mask);

#ifdef NUMA_SUPPORT
    int ncpus = numa_num_configured_cpus();
    int nodes = numa_max_node() + 1;
    int node_size = ncpus / nodes;

    if (numa_node_of_cpu(0) == numa_node_of_cpu(ncpus/2)) {
        __prefered_core = ((cpu_id % node_size) * nodes + (cpu_id/node_size))% USE_CPUS;
        CPU_SET(__prefered_core, &mask);
    } else {
        __prefered_core = ((cpu_id % nodes) * node_size)% USE_CPUS;
        CPU_SET(__prefered_core, &mask);
    }

#   ifdef DEBUG
    fprintf(stderr, "DEBUG: thread: %d -- numa_node: %d -- core: %d -- nodes: %d\n", cpu_id, numa_node_of_cpu(__prefered_core), __prefered_core, nodes);
#   endif

#else
    CPU_SET(cpu_id % USE_CPUS, &mask);
#endif
    ret = sched_setaffinity(0, len, &mask);
    if (ret == -1)
        perror("sched_setaffinity");

    return ret;
#endif
}

inline static void *uthreadWrapper(void *arg) {
    int i, kernel_id;
    long pid = (long)arg;

    kernel_id = (pid / __uthreads) % USE_CPUS;
    threadPin(kernel_id);
    setThreadId(kernel_id);
    start_cpu_counters(kernel_id);
    initFibers(__uthreads);
    for (i = 0; i < __uthreads-1; i++) {
        spawnFiber(__func, pid + i + 1);
    }
    __func((void *)pid);

    waitForAllFibers();
    stop_cpu_counters(kernel_id);
    BarrierLeave(&bar);
    return null;
}

int StartThreadsN(int nthreads, void *(*func)(void *), int mode) {
    long i;
    int last_thread_id;

    init_cpu_counters();
    __nthreads = nthreads;
    __threads = getMemory(nthreads * sizeof(pthread_t));
    __func = func;
    StoreFence(); 
    if (mode == _USE_UTHREADS_ && FIBERS_PER_THREAD > 1) {
        long uthreads = FIBERS_PER_THREAD;

        __uthreads = uthreads;
        __uthread_sched = true;
        BarrierInit(&bar, nthreads/uthreads + 1);
        for (i = 0; i < (nthreads/uthreads)-1; i++) {
            last_thread_id = pthread_create(&__threads[i], null, uthreadWrapper, (void *)(i*uthreads));
            if (last_thread_id != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
            __unjoined_threads++;
        }
        uthreadWrapper((void *)(i*uthreads));
    } else {
        __uthread_sched = false;
        BarrierInit(&bar, nthreads + 1);
        for (i = 0; i < nthreads-1; i++) {
            last_thread_id = pthread_create(&__threads[i], null, kthreadWrapper, (void *)i);
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

void JoinThreadsN(int nthreads) {
    BarrierWait(&bar);
    free(__threads);
}

int32_t getThreadId(void) {
    return __thread_id;
}

void resched(void) {
    if (__uthread_sched)
        fiberYield();
    else if (__nthreads <= USE_CPUS)
        ;
    else sched_yield();
}

bool isSystemOversubscribed(void) {
    if (__nthreads > USE_CPUS) return true;
    else return false;
}
