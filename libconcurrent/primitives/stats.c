#include <stdio.h>
#include <stats.h>
#include <primitives.h>
#include <threadtools.h>

#ifdef DEBUG
#    include <types.h>
#    include <system.h>

__thread int64_t __failed_cas CACHE_ALIGN = 0;
__thread int64_t __executed_cas CACHE_ALIGN = 0;
__thread int64_t __executed_swap CACHE_ALIGN = 0;
__thread int64_t __executed_faa CACHE_ALIGN = 0;

volatile int64_t __total_failed_cas = 0;
volatile int64_t __total_executed_cas = 0;
volatile int64_t __total_executed_swap = 0;
volatile int64_t __total_executed_faa = 0;

#endif

#ifdef _TRACK_CPU_COUNTERS
#    include <system.h>
#    include <stdlib.h>
#    include <pthread.h>
#    include <papi.h>

#    define N_CPU_COUNTERS 4

static volatile int *__cpu_events = NULL;
static volatile long long **__cpu_values = NULL;
#endif

void synchInitCPUCounters(void) {
#ifdef _TRACK_CPU_COUNTERS
    const PAPI_hw_info_t *hwinfo = NULL;
    int ret, i;

    while (__cpu_events == NULL) {
        void *ptr = getAlignedMemory(CACHE_LINE_SIZE, synchGetNCores() * sizeof(int));
        if (CASPTR(&__cpu_events, NULL, ptr) == false) synchFreeMemory(ptr, synchGetNCores() * sizeof(int));
    }

    while (__cpu_values == NULL) {
        void *ptr = getAlignedMemory(CACHE_LINE_SIZE, synchGetNCores() * sizeof(long long *));
        if (CASPTR(&__cpu_values, NULL, ptr) == false) synchFreeMemory(ptr, synchGetNCores() * sizeof(long long *));
    }

    for (i = 0; i < synchGetNCores(); i++) {
        while (__cpu_values[i] == NULL) {
            void *ptr = getAlignedMemory(CACHE_LINE_SIZE, N_CPU_COUNTERS * sizeof(long long));
            if (CASPTR(&__cpu_values[i], NULL, ptr) == false) synchFreeMemory(ptr, N_CPU_COUNTERS * sizeof(long long));
        }
    }
    if ((ret = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT && ret > 0) {
        fprintf(stderr, "PAPI ERROR: unable to initialize PAPI library\n");
        exit(EXIT_FAILURE);
    }

    PAPI_thread_init(pthread_self);

    if ((hwinfo = PAPI_get_hardware_info()) == NULL)
        exit(1);

    fprintf(stderr, "\n\n%d CPUs at %f MHz.\n", hwinfo->totalcpus, hwinfo->mhz);
#endif
}

void synchStartCPUCounters(int id) {
#ifdef DEBUG
    __failed_cas = 0;
    __executed_cas = 0;
    __executed_swap = 0;
    __executed_faa = 0;
#endif

#ifdef _TRACK_CPU_COUNTERS
    __cpu_events[id] = PAPI_NULL;

    if (PAPI_create_eventset((int *)&__cpu_events[id]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to initialize performance counters\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_L1_DCM) != PAPI_OK) {
        if (id == 0)
            fprintf(stderr, "PAPI WARNING: unable to create event for L1 data cache misses\n");
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_L2_DCM) != PAPI_OK) {
        if (id == 0)
            fprintf(stderr, "PAPI WARNING: unable to create event for L2 data cache misses\n");
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_BR_MSP) != PAPI_OK) {
        if (id == 0)
            fprintf(stderr, "PAPI WARNING: unable to create event for branch mis-predictions\n");
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_RES_STL) != PAPI_OK) {
        if (id == 0)
            fprintf(stderr, "PAPI WARNING: unable to create event for cpu stalls\n");
    }
    if (PAPI_start(__cpu_events[id]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to start performance counters\n");
        exit(EXIT_FAILURE);
    }
#endif
}

void synchStopCPUCounters(int id) {
#ifdef DEBUG
    FAA64(&__total_failed_cas, __failed_cas);
    FAA64(&__total_executed_cas, __executed_cas);
    FAA64(&__total_executed_swap, __executed_swap);
    FAA64(&__total_executed_faa, __executed_faa);
#endif

#ifdef _TRACK_CPU_COUNTERS
    if (PAPI_read(__cpu_events[id], (long long *)__cpu_values[id]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to read counters\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_stop(__cpu_events[id], (long long *)__cpu_values[id]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to stop counters\n");
        exit(EXIT_FAILURE);
    }
#endif
}

void synchPrintStats(uint32_t nthreads, uint64_t runs) {
#ifdef DEBUG
    printf("DEBUG: ");
    printf("failed_CAS_per_op: %f\t", (float)__total_failed_cas / runs);
    printf("executed_CAS: %ld\t", __total_executed_cas);
    printf("successful_CAS: %ld\t", __total_executed_cas - __total_failed_cas);
    printf("executed_SWAP: %ld\t", __total_executed_swap);
    printf("executed_FAA: %ld\t", __total_executed_faa);
    printf("atomics: %ld\t", __total_executed_cas + __total_executed_swap + __total_executed_faa);
    printf("atomics_per_op: %.2f\t", ((float)(__total_executed_cas + __total_executed_swap + __total_executed_faa)) / runs);
    printf("operations_per_CAS: %.2f", runs / ((float)(__total_executed_cas - __total_failed_cas)));
#endif
    printf("\n");

#ifdef _TRACK_CPU_COUNTERS
    long long __total_cpu_values[N_CPU_COUNTERS];
    int k, j;
    double ops = runs * nthreads;

    for (j = 0; j < N_CPU_COUNTERS; j++) {
        __total_cpu_values[j] = 0;
        for (k = 0; k < synchGetNCores(); k++)
            __total_cpu_values[j] += __cpu_values[k][j];
    }

    fprintf(stderr,
            "DEBUG: L1 data cache misses: %.2lf\t"
            "L2 data cache misses: %.2lf\t"
            "Branch mis-predictions: %.2lf\t"
            "CPU stalls: %.2lf\t total operations: %ld\n",
            __total_cpu_values[0] / ops,
            __total_cpu_values[1] / ops,
            __total_cpu_values[2] / ops,
            __total_cpu_values[3] / ops,
            (long)ops);
#endif
}
