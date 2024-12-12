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
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

static void *Execute(void *Arg) {
    long i, rnum, mybank;
    volatile long j;
    int id = synchGetThreadId();
    ToggleVector lactive_set;
    ToggleVector mystate;

    synchFastRandomSetSeed(id + 1);
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    TVEC_INIT(&mystate, bench_args.nthreads);
    TVEC_INIT(&lactive_set, bench_args.nthreads);
    TVEC_SET_BIT(&mystate, id);
    mybank = TVEC_GET_BANK_OF_BIT(id, bench_args.nthreads);
    for (i = 0; i < bench_args.runs; i++) {
        TVEC_NEGATIVE(&mystate, &mystate);
        TVEC_ATOMIC_ADD_BANK(&active_set, &mystate, mybank);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        TVEC_COPY(&lactive_set, (void *)&active_set);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    synchBarrierWait(&bar);
    if (id == 0) d2 = synchGetTimeMillis();

    return NULL;
}

int main(int argc, char *argv[]) {
    synchParseArguments(&bench_args, argc, argv);
    TVEC_INIT((ToggleVector *)&active_set, bench_args.nthreads);
    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);

    return 0;
}
