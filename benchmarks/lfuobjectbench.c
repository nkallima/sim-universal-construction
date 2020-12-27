#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <backoff.h>
#include <threadtools.h>
#include <lfuobject.h>
#include <barrier.h>
#include <bench_args.h>
#include <fam.h>

LFUObject lfobject CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
int MIN_BAK, MAX_BAK;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    LFUObjectThreadState *th_state;
    long i, rnum;
    volatile long j;
    long id = (long)Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LFUObjectThreadState));
    LFUObjectThreadStateInit(th_state, bench_args.backoff_low, bench_args.backoff_high);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        LFUObjectApplyOp(&lfobject, th_state, fetchAndMultiply, 1, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    lfobject.state.state_f = 1.0;
    LFUObjectInit(&lfobject, (ArgVal)lfobject.state.state);
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: Object state: %f\n", lfobject.state.state_f);
#endif

    return 0;
}
