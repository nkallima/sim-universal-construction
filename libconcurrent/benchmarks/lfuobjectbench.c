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

LFUObject lfobject CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
int MIN_BAK, MAX_BAK;
Barrier bar;

inline static RetVal fetchAndMultiply(Object mod_sp, Object arg, int pid);


inline static RetVal fetchAndMultiply(Object mod_sp, Object arg, int pid) {
     mod_sp +=1;//*= arg;
     return (RetVal)mod_sp;
}

inline static void *Execute(void* Arg) {
    LFUObjectThreadState *th_state;
    long i, rnum;
    volatile long j;
    long id = (long) Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LFUObjectThreadState));
    LFUObjectThreadStateInit(th_state, MIN_BAK, MAX_BAK);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
        LFUObjectApplyOp(&lfobject, th_state, fetchAndMultiply, (Object) (id + 1), id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Please set upper and lower bound for backoff!\n");
        exit(EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &MIN_BAK);
        sscanf(argv[2], "%d", &MAX_BAK);
    }

    LFUObjectInit(&lfobject, (ArgVal)1);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);
    return 0;
}


