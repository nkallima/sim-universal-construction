#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <uthreads.h>
#include <osci.h>
#include <barrier.h>
#include <bench_args.h>

volatile Object object CACHE_ALIGN = 1;
OsciStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;


inline static RetVal fetchAndMultiply(void *state, ArgVal arg, int pid) {
    Object *st = (Object *)state;
    (*st) *= arg;
    return *st;
}

inline static void *Execute(void* Arg) {
    OsciThreadState *th_state;
    long i, rnum;
    volatile int j;
    long pid = (long)Arg;

    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(OsciThreadState));
    fastRandomSetSeed(pid);
    OsciThreadStateInit(th_state, &object_lock, pid);
    BarrierWait(&bar);
    if (pid == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        OsciApplyOp(&object_lock, th_state, fetchAndMultiply, (void *)&object, (ArgVal) pid, pid);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ; 
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);

    OsciInit(&object_lock, bench_args.nthreads, bench_args.fibers_per_thread);
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: object counter: %d\n", object_lock.counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", object_lock.rounds);
    fprintf(stderr, "DEBUG: Average helping: %f\n", (float)object_lock.counter/object_lock.rounds);
    fprintf(stderr, "\n");
#endif

    return 0;
}
