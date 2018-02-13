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

int64_t d1 CACHE_ALIGN, d2;
volatile ToggleVector active_set;
Barrier bar;

inline static void *Execute(void* Arg) {
    long i, rnum, mybank;
    volatile long j;
    long id = (long) Arg;
    volatile ToggleVector lactive_set;
    ToggleVector mystate;

    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();
    TVEC_SET_ZERO(&mystate);
    TVEC_SET_BIT(&mystate, id);
    mybank = TVEC_GET_BANK_OF_BIT(id);
    for (i = 0; i < RUNS; i++) {
        mystate = TVEC_NEGATIVE(mystate);
        TVEC_ATOMIC_ADD_BANK(&active_set, &mystate, mybank);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        lactive_set = active_set;
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    TVEC_SET_ZERO((ToggleVector *)&active_set);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2*RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

    return 0;
}
