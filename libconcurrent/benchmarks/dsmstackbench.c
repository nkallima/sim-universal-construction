#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <dsmstack.h>
#include <barrier.h>

DSMStackStruct object_struct CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar;

inline static void *Execute(void* Arg) {
    DSMStackThreadState *th_state;
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(DSMStackThreadState));
    DSMStackThreadStateInit(&object_struct, th_state, (int)id);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
        // perform a push operation
        DSMStackPush(&object_struct, th_state, id, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
        // perform a pop operation
        DSMStackPop(&object_struct, th_state, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(void) {
    DSMSStackInit(&object_struct);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2*RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

#ifdef DEBUG
    fprintf(stderr, "object state:    counter: %d rounds: %d\n", object_struct.object_struct.counter, object_struct.object_struct.rounds);
#endif

    return 0;
}
