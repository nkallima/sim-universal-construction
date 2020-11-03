#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <tvec.h>
#include <fastrand.h>
#include <backoff.h>
#include <threadtools.h>
#include <barrier.h>
#include <bench_args.h>

int64_t d1 CACHE_ALIGN, d2;
volatile ToggleVector active_set CACHE_ALIGN;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void* Arg) {
    long i, rnum, mybank;
    volatile long j;
    long id = (long) Arg;
    ToggleVector lactive_set;
    ToggleVector mystate;

    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();
    TVEC_INIT(&mystate, bench_args.nthreads);
    TVEC_INIT(&lactive_set, bench_args.nthreads);
    TVEC_SET_BIT(&mystate, id);
    mybank = TVEC_GET_BANK_OF_BIT(id, bench_args.nthreads);
    for (i = 0; i < bench_args.runs; i++) {
        TVEC_NEGATIVE(&mystate, &mystate);
        TVEC_ATOMIC_ADD_BANK(&active_set, &mystate, mybank);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        TVEC_COPY(&lactive_set, (void *)&active_set);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    TVEC_INIT((ToggleVector *)&active_set, bench_args.nthreads);
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

    return 0;
}
