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
#include <bench_args.h>

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
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

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
    
    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        apply_op(fetchAndMultiply, (ArgVal) i, (int)id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    object_lock = CLHLockInit(bench_args.nthreads);

    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

    return 0;
}
