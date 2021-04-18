#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <sim.h>
#include <barrier.h>
#include <bench_args.h>
#include <fam.h>
#include <fastrand.h>
#include <threadtools.h>

SimStruct *sim_struct CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;
int MAX_BACK CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    SimThreadState th_state;
    long i, rnum;
    long id = (long)Arg;
    volatile long j;

    SimThreadStateInit(&th_state, bench_args.nthreads, id);
    fastRandomSetSeed((unsigned long)id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        SimApplyOp(sim_struct, &th_state, fetchAndMultiply, (Object)(id + 1), id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    BarrierWait(&bar);
    if (id == 0) d2 = getTimeMillis();

    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    sim_struct = getAlignedMemory(CACHE_LINE_SIZE, sizeof(SimStruct));
    SimInit(sim_struct, bench_args.nthreads, bench_args.backoff_high);
    BarrierSet(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    SimObjectState *l = (SimObjectState *)sim_struct->pool[((pointer_t *)&sim_struct->sp)->struct_data.index];
    fprintf(stderr, "DEBUG: Object state: %d\n", l->counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", l->rounds);
    fprintf(stderr, "DEBUG: Average helping: %f\n", (float)l->counter / l->rounds);
#endif

    return 0;
}
