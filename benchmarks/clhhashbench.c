#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <clhhash.h>
#include <barrier.h>
#include <bench_args.h>
#include <math.h>

#define N_BUCKETS            128
#define INITIAL_LOAD_FACTOR  2
#define INITIAL_CAPACITY     (INITIAL_LOAD_FACTOR * N_BUCKETS)
#define RANDOM_RANGE         1000
#define RANDOM_RANGE_MIN(ID) (RANDOM_RANGE * (ID) + 1)
#define RANDOM_RANGE_MAX(ID) (RANDOM_RANGE * (ID + 1) - 1)

CLHHash object_struct CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    int64_t key, value;
    CLHHashThreadState *th_state;
    long i, rnum;
    volatile int j;
    int id = synchGetThreadId();

    synchFastRandomSetSeed(id + 1);
    th_state = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(CLHHashThreadState));
    CLHHashThreadStateInit(&object_struct, th_state, N_BUCKETS, (int)id);
#ifndef DEBUG
    if (id == 0) {
        for (i = 0; i < INITIAL_CAPACITY; i++) {
            key = synchFastRandomRange32(RANDOM_RANGE_MIN(0), RANDOM_RANGE_MAX(bench_args.nthreads));
            value = id;
            CLHHashInsert(&object_struct, th_state, key, value, id);
        }
    }
#endif
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        key = synchFastRandomRange32(RANDOM_RANGE_MIN(id), RANDOM_RANGE_MAX(id));
        value = id;
        CLHHashInsert(&object_struct, th_state, key, value, id);
        CLHHashDelete(&object_struct, th_state, key, id);
        CLHHashSearch(&object_struct, th_state, key, id);
#ifdef DEBUG
        bool found = CLHHashSearch(&object_struct, th_state, key, id);
        if (found)
            fprintf(stderr, "DEBUG: Found key: %ld - thread: %d - iteration: %ld\n", key, id, i);
#endif
    }
    synchBarrierWait(&bar);
    if (id == 0) d2 = synchGetTimeMillis();

    return NULL;
}

int main(int argc, char *argv[]) {
    synchParseArguments(&bench_args, argc, argv);
    CLHHashStructInit(&object_struct, N_BUCKETS, bench_args.nthreads);

    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 3 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);

    return 0;
}
