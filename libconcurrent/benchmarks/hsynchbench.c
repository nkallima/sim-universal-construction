#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <hsynch.h>
#include <threadtools.h>
#include <barrier.h>
#include <bench_args.h>

volatile Object object CACHE_ALIGN = 1;
HSynchStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static RetVal fetchAndMultiply(void *state, ArgVal arg, int pid) {
    Object *st = (Object *)state;
    (*st) *= arg;
    return *st;
}

inline static void *Execute(void* Arg) {
    HSynchThreadState *th_state;
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HSynchThreadState));
    HSynchThreadStateInit(th_state, (int)id);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform a fetchAndMultiply operation
        HSynchApplyOp(&object_lock, th_state, fetchAndMultiply, (void *)&object, (ArgVal) id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ; 
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);

    HSynchStructInit(&object_lock, bench_args.nthreads); 
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

#ifdef DEBUG
    fprintf(stderr, "object state:    counter: %d rounds: %d\n", object_lock.counter, object_lock.rounds);
#endif

    return 0;
}
