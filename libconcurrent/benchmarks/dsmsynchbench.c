#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <dsmsynch.h>
#include <barrier.h>
#include <bench_args.h>
#include <fam.h>

volatile ObjectState *object CACHE_ALIGN;
DSMSynchStruct *object_combiner;
int64_t d1, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void* Arg) {
    DSMSynchThreadState *th_state;
    long i, rnum;
    volatile long j;
    long id = (long) Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(DSMSynchThreadState));
    DSMSynchThreadStateInit(object_combiner, th_state, (int)id);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform a fetchAndMultiply operation
        DSMSynchApplyOp(object_combiner, th_state, fetchAndMultiply, (void *)object, (ArgVal) id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    object = getAlignedMemory(S_CACHE_LINE_SIZE, sizeof(ObjectState));
    object_combiner = getAlignedMemory(CACHE_LINE_SIZE, sizeof(DSMSynchStruct));
    object->state_f = 1.0;
    DSMSynchStructInit(object_combiner, bench_args.nthreads);

    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: object state: %f\n", object->state_f);
    fprintf(stderr, "DEBUG: object counter: %d\n", object_combiner->counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", object_combiner->rounds);
    fprintf(stderr, "DEBUG: Average helping: %f\n", (float)object_combiner->counter/object_combiner->rounds);
#endif

    return 0;
}
