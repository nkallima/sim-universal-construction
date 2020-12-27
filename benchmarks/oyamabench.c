#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <pool.h>
#include <threadtools.h>
#include <oyama.h>
#include <barrier.h>
#include <bench_args.h>

volatile Object object CACHE_ALIGN = 1;
volatile OyamaStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static RetVal fetchAndMultiply(ArgVal arg, int pid);

inline static RetVal fetchAndMultiply(ArgVal arg, int pid) {
    Object old_val;

    old_val = object;
    object += 1;
    return old_val;
}

inline static void *Execute(void *Arg) {
    OyamaThreadState *th_state;
    long i, rnum;
    volatile int j;
    long id = (long)Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(OyamaThreadState));
    OyamaThreadStateInit(th_state);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform a fetchAndMultiply operation
        OyamaApplyOp((OyamaStruct *)&object_lock, th_state, fetchAndMultiply, (ArgVal)id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);

    OyamaInit((OyamaStruct *)&object_lock, bench_args.nthreads);
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: object counter: %d\n", object_lock.counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", object_lock.rounds);
    fprintf(stderr, "DEBUG: Average helping: %f\n", (float)object_lock.counter / object_lock.rounds);
#endif

    return 0;
}
