#ifndef _BENCH_ARGS_H_
#define _BENCH_ARGS_H_

#include <stdint.h>


typedef struct BenchArgs {
    uint64_t runs;
    uint32_t nthreads;
    uint32_t fibers_per_thread;
    uint32_t max_work;
    uint32_t numa_nodes;
    uint16_t backoff_low;
    uint16_t backoff_high;
} BenchArgs;


void parseArguments(BenchArgs *bench_args, int argc, char *argv[]);

#endif
