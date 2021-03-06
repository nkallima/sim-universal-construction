#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <hstack.h>
#include <barrier.h>
#include <bench_args.h>

HStackStruct *object_struct CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    HStackThreadState *th_state;
    long i, rnum;
    volatile int j;
    long id = (long)Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HStackThreadState));
    HStackThreadStateInit(object_struct, th_state, (int)id);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform a push operation
        HStackPush(object_struct, th_state, id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        // perform a pop operation
        HStackPop(object_struct, th_state, id);
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
    object_struct = getAlignedMemory(S_CACHE_LINE_SIZE, sizeof(HStackStruct));
    HStackInit(object_struct, bench_args.nthreads, bench_args.numa_nodes);
    BarrierSet(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: Object state: %ld\n", object_struct->object_struct.counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", object_struct->object_struct.rounds);
    volatile Node *top = object_struct->top;
    long counter;

    counter = 0;
    while (top != null) {
        top = top->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes left in the queue\n", counter);
#endif

    return 0;
}
