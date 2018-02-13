#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <ccsynch.h>
#include <barrier.h>

volatile Object object CACHE_ALIGN;
CCSynchStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar;

inline static RetVal fetchAndMultiply(void *state, ArgVal arg, int pid) {
    Object *st = (Object *)state;
    (*st) *= arg;
    return *st;
}

inline static void *Execute(void* Arg) {
    CCSynchThreadState *th_state;
    long i, rnum;
    volatile long j;
    long id = (long) Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CCSynchThreadState));
    CCSynchThreadStateInit(th_state, (int)id);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
        // perform a fetchAndMultiply operation
        CCSynchApplyOp(&object_lock, th_state, fetchAndMultiply, (void *)&object, (ArgVal) id, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(void) {
    object = 1;
    CCSynchStructInit(&object_lock);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

#ifdef DEBUG
    fprintf(stderr, "object counter: %d\n", object_lock.counter);
    fprintf(stderr, "rounds: %d\n", object_lock.rounds);
    fprintf(stderr, "Average helping: %.2f\n", (float)object_lock.counter/object_lock.rounds);
    fprintf(stderr, "\n");
#endif

    return 0;
}
