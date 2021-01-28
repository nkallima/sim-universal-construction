#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <backoff.h>
#include <fastrand.h>
#include <threadtools.h>
#include <msqueue.h>
#include <barrier.h>
#include <bench_args.h>

MSQueue queue CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
int MIN_BAK, MAX_BAK;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    MSQueueThreadState *th_state;
    long i;
    long id = (long)Arg;
    long rnum;
    volatile long j;

    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(MSQueueThreadState));
    MSQueueThreadStateInit(th_state, bench_args.backoff_low, bench_args.backoff_high);
    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        MSQueueEnqueue(&queue, th_state, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        MSQueueDequeue(&queue, th_state);
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

    MSQueueInit(&queue);
    BarrierSet(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);
#ifdef DEBUG
    long counter = 0;

    while (queue.head != null) {
        queue.head = queue.head->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the queue\n", counter - 1);
#endif

    return 0;
}
