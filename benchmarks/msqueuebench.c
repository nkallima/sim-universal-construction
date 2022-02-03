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

MSQueueStruct queue CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
int MIN_BAK, MAX_BAK;
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    MSQueueThreadState *th_state;
    long i;
    long id = (long)Arg;
    long rnum;
    volatile long j;

    th_state = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(MSQueueThreadState));
    MSQueueThreadStateInit(th_state, bench_args.backoff_low, bench_args.backoff_high);
    synchFastRandomSetSeed(id + 1);
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        MSQueueEnqueue(&queue, th_state, id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        MSQueueDequeue(&queue, th_state);
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

    MSQueueInit(&queue);
    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);
#ifdef DEBUG
    long counter = 0;

    while (queue.head != NULL) {
        queue.head = queue.head->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the queue\n", counter - 1);
#endif

    return 0;
}
