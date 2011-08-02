#ifndef _STATS_H_
#define _STATS_H_

#ifdef DEBUG
#include "types.h"

int_aligned32_t __failed_cas[N_THREADS] CACHE_ALIGN;
int_aligned32_t __executed_cas[N_THREADS] CACHE_ALIGN;
int_aligned32_t __executed_swap[N_THREADS] CACHE_ALIGN;
int_aligned32_t __executed_faa[N_THREADS] CACHE_ALIGN;

__thread int __stats_thread_id;
#endif

#ifdef _TRACK_CPU_COUNTERS
#include "papi.h"

int __cpu_events[4] = {PAPI_L1_TCM, PAPI_L1_DCM, PAPI_BR_TKN, PAPI_BR_MSP};
long long __cpu_values[N_THREADS][4] CACHE_ALIGN;
#endif

void init_cpu_counters(void) {
#ifdef _TRACK_CPU_COUNTERS
    unsigned long int tid;

    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
        exit(EXIT_FAILURE);
#endif
}
 
void start_cpu_counters(int id) {
#ifdef DEBUG
     __stats_thread_id = id;
     __failed_cas[id].v = 0;
     __executed_cas[id].v = 0;
     __executed_swap[id].v = 0;
#endif

#ifdef _TRACK_CPU_COUNTERS
     if (PAPI_start_counters(__cpu_events, 4) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to initialize counters\n");
        exit(EXIT_FAILURE);
     }
#endif
}

void stop_cpu_counters(int id) {
#ifdef _TRACK_CPU_COUNTERS
    int i;

    if (PAPI_stop_counters(__cpu_values[id], 4) != PAPI_OK) {
        fprintf(stderr, "PAPI ERROR: unable to stop counters\n");
        exit(EXIT_FAILURE);
    }
#endif
}


void printStats(void) {
#ifdef DEBUG
    int i;
    int __total_failed_cas = 0;
    int __total_executed_cas = 0;
    int __total_executed_swap = 0;
    int __total_executed_faa = 0;

    for (i = 0; i < N_THREADS; i++) {
        __total_failed_cas += __failed_cas[i].v;
        __total_executed_cas += __executed_cas[i].v;
        __total_executed_swap += __executed_swap[i].v;
        __total_executed_faa += __executed_faa[i].v;
    }

    printf("failed_CAS_per_op: %f\t", (float)__total_failed_cas/(N_THREADS * RUNS));
    printf("executed_CAS: %d\t", __total_executed_cas);
    printf("successful_CAS: %d\t", __total_executed_cas - __total_failed_cas);
    printf("executed_SWAP: %d\t", __total_executed_swap);
    printf("executed_FAA: %d\t", __total_executed_faa);
    printf("atomics: %d\t", __total_executed_cas + __total_executed_swap + __total_executed_faa);
    printf("atomics_per_op: %.2f\t", ((float)(__total_executed_cas + __total_executed_swap + __total_executed_faa))/(N_THREADS * RUNS));
    printf("operations_per_CAS: %.2f", (N_THREADS * RUNS)/((float)(__total_executed_cas - __total_failed_cas)));
#endif
    printf("\n");

#ifdef _TRACK_CPU_COUNTERS
    long long __total_cpu_values[4];
    for (j = 0; j < 4; j++) {
        __total_cpu_values[j] = 0;
        for (i = 0; i < N_THREADS; i++)
            __total_cpu_values[j] += __cpu_values[i][j];
    }

    fprintf(stderr, "L1 total cache misses: %lld\t"
                "L1 data cache misses: %lld\t"
                "Total branches executed: %lld\t"
                "Mispredicted branches: %lld\n",
              __total_cpu_values[0], __total_cpu_values[1], __total_cpu_values[2], __total_cpu_values[3]);
#endif
}


#endif
