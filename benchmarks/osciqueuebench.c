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
#include <osciqueue.h>
#include <barrier.h>
#include <bench_args.h>

OsciQueueStruct queue_object CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    OsciQueueThreadState *th_state;
    long i, rnum;
    volatile int j;
    long pid = (long)Arg;

    fastRandomSetSeed(pid);
    BarrierWait(&bar);
    if (pid == 0) {
        d1 = getTimeMillis();
    }
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(OsciQueueThreadState));
    OsciQueueThreadStateInit(&queue_object, th_state, pid);
    for (i = 0; i < bench_args.runs; i++) {
        // perform an enqueue operation
        OsciQueueApplyEnqueue(&queue_object, th_state, (ArgVal)pid, pid);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        // perform a dequeue operation
        OsciQueueApplyDequeue(&queue_object, th_state, pid);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);

    OsciQueueInit(&queue_object, bench_args.nthreads, bench_args.fibers_per_thread);
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: enqueue state: counter: %d rounds: %d\n", queue_object.enqueue_struct.counter, queue_object.enqueue_struct.rounds);
    fprintf(stderr, "DEBUG: dequeue state: counter: %d rounds: %d\n\n", queue_object.dequeue_struct.counter, queue_object.dequeue_struct.rounds);
#endif

    return 0;
}
