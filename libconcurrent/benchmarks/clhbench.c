#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <clh.h>
#include <threadtools.h>
#include <barrier.h>

typedef struct ObjectState {
    long long state;
} ObjectState;

typedef union CRStruct {
    volatile Object obj;
    char pad[CACHE_LINE_SIZE];
} CRStruct;

CLHLockStruct *object_lock CACHE_ALIGN;
CRStruct Critical[OBJECT_SIZE] CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar;

inline RetVal fetchAndMultiply(ArgVal arg, int pid) {
    int i;

    for (i = 0; i < OBJECT_SIZE; i++)
        Critical[i].obj += 1;
    return Critical[0].obj;
}

inline void apply_op(RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid) {
    CLHLock(object_lock, pid);
    sfunc(arg, pid);
    CLHUnlock(object_lock, pid);
}

inline static void *Execute(void* Arg) {
    long i, rnum;
    volatile long j;
    long id = (long) Arg;
    
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif

#ifdef sun
    schedule_control = schedctl_init();
#endif
    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
#if defined(__sun) || defined(sun)
        schedctl_start(schedule_control);
#endif
        apply_op(fetchAndMultiply, (ArgVal) i, (int)id);
#if defined(__sun) || defined(sun)
        schedctl_stop(schedule_control);
#endif
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(void) {
    object_lock = CLHLockInit(N_THREADS);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

    return 0;
}
