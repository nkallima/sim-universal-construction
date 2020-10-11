#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <uthreads.h>
#include <osci.h>
#include <barrier.h>


volatile Object object CACHE_ALIGN = 1;
OsciStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar;


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

    for (i = 0; i < RUNS; i++) {
        OsciApplyOp(&object_lock, th_state, fetchAndMultiply, (void *)&object, (ArgVal) pid, pid);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
    }
    return NULL;
}

int main(void) {
    OsciInit(&object_lock, N_THREADS, N_THREADS/getNCores());
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _USE_UTHREADS_);
    JoinThreadsN(N_THREADS - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: object counter: %d\n", object_lock.counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", object_lock.rounds);
    fprintf(stderr, "DEBUG: Average helping: %f\n", (float)object_lock.counter/object_lock.rounds);
    fprintf(stderr, "\n");
#endif

    return 0;
}
