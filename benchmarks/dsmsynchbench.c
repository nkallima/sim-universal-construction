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
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    DSMSynchThreadState *th_state;
    long i, rnum;
    volatile long j;
    int id = synchGetThreadId();

    synchFastRandomSetSeed(id + 1);
    th_state = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(DSMSynchThreadState));
    DSMSynchThreadStateInit(object_combiner, th_state, (int)id);
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform a fetchAndMultiply operation
        DSMSynchApplyOp(object_combiner, th_state, fetchAndMultiply, (void *)object, (ArgVal)id, id);
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
    object = synchGetAlignedMemory(S_CACHE_LINE_SIZE, sizeof(ObjectState));
    object_combiner = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(DSMSynchStruct));
    object->state_f = 1.0;
    DSMSynchStructInit(object_combiner, bench_args.nthreads);

    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: Object state: %ld\n", object_combiner->counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", object_combiner->rounds);
    fprintf(stderr, "DEBUG: Average helping: %f\n", (float)object_combiner->counter / object_combiner->rounds);
#endif

    return 0;
}
