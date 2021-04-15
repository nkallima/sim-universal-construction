#ifndef _STATS_H_
#define _STATS_H_

#include <stdint.h>

void init_cpu_counters(void);
void start_cpu_counters(int id);
void stop_cpu_counters(int id);
void printStats(uint32_t nthreads, uint64_t runs);

#endif
