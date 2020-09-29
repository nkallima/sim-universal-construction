#include <stdio.h>
#include <stats.h>

#ifdef DEBUG
#include <types.h>
#include <system.h>

#include <primitives.h>

__thread int64_t __failed_cas CACHE_ALIGN  = 0;
__thread int64_t __executed_cas CACHE_ALIGN = 0;
__thread int64_t __executed_swap CACHE_ALIGN = 0;
__thread int64_t __executed_faa CACHE_ALIGN = 0;

volatile int64_t __total_failed_cas = 0;
volatile int64_t __total_executed_cas = 0;
volatile int64_t __total_executed_swap = 0;
volatile int64_t __total_executed_faa = 0;

#endif

#ifdef _TRACK_CPU_COUNTERS
#include <system.h>
#include <papi.h>

int __cpu_events[USE_CPUS] CACHE_ALIGN;
long long __cpu_values[USE_CPUS][4] CACHE_ALIGN;
#endif

void init_cpu_counters(void) {
#ifdef _TRACK_CPU_COUNTERS
    const PAPI_hw_info_t *hwinfo = NULL;
    int ret;

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
 
void start_cpu_counters(int id) {
#ifdef DEBUG
    __failed_cas = 0;
    __executed_cas = 0;
    __executed_swap = 0;
    __executed_faa = 0;
#endif
     
#ifdef _TRACK_CPU_COUNTERS
    __cpu_events[id] = PAPI_NULL;
 
    if (PAPI_create_eventset(&__cpu_events[id]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to initialize performance counters\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_L1_DCM) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to create event for L1 data cache misses\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_L2_DCM) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to create event for L2 data cache misses\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_BR_MSP) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to create event for branch mis-predictions\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_add_event(__cpu_events[id], PAPI_RES_STL) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to create event for cpu stalls\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_start(__cpu_events[id]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to start performance counters\n");
        exit(EXIT_FAILURE);
    }
#endif
}

void stop_cpu_counters(int id) {
#ifdef DEBUG
        FAA64(&__total_failed_cas, __failed_cas);
        FAA64(&__total_executed_cas, __executed_cas);
        FAA64(&__total_executed_swap, __executed_swap);
        FAA64(&__total_executed_faa, __executed_faa);
#endif

#ifdef _TRACK_CPU_COUNTERS
    if (PAPI_read(__cpu_events[id], &__cpu_values[id][0]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to read counters\n");
        exit(EXIT_FAILURE);
    }
    if (PAPI_stop(__cpu_events[id], &__cpu_values[id][0]) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to stop counters\n");
        exit(EXIT_FAILURE);
    }
#endif
}


void printStats(int nthreads) {
#ifdef DEBUG
    printf("DEBUG: ");
    printf("failed_CAS_per_op: %f\t", (float)__total_failed_cas/(nthreads * RUNS));
    printf("executed_CAS: %ld\t", __total_executed_cas);
    printf("successful_CAS: %ld\t", __total_executed_cas - __total_failed_cas);
    printf("executed_SWAP: %ld\t", __total_executed_swap);
    printf("executed_FAA: %ld\t", __total_executed_faa);
    printf("atomics: %ld\t", __total_executed_cas + __total_executed_swap + __total_executed_faa);
    printf("atomics_per_op: %.2f\t", ((float)(__total_executed_cas + __total_executed_swap + __total_executed_faa))/(nthreads * RUNS));
    printf("operations_per_CAS: %.2f", (nthreads * RUNS)/((float)(__total_executed_cas - __total_failed_cas)));
#endif
    printf("\n");

#ifdef _TRACK_CPU_COUNTERS
    long long __total_cpu_values[4];
    int k, j;
    double ops = RUNS * nthreads;

    for (j = 0; j < 4; j++) {
        __total_cpu_values[j] = 0;
        for (k = 0; k < USE_CPUS; k++)
            __total_cpu_values[j] += __cpu_values[k][j];
    }


    fprintf(stderr, "DEBUG: L1 data cache misses: %.2lf\t"
            "L2 data cache misses: %.2lf\t"
            "Branch mis-predictions: %.2lf\t"
            "CPU stalls: %.2lf\t total operations: %ld\n",
            __total_cpu_values[0]/ops, __total_cpu_values[1]/ops,
            __total_cpu_values[2]/ops, __total_cpu_values[3]/ops, (long)ops);
#endif
}
