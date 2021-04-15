#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <sched.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <hqueue.h>
#include <barrier.h>
#include <bench_args.h>

HQueueStruct *queue_object CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    HQueueThreadState *th_state;
    long i, rnum;
    volatile int j;
    long id = (long)Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HQueueThreadState));
    HQueueThreadStateInit(queue_object, th_state, (int)id);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform an enqueue operation
        HQueueApplyEnqueue(queue_object, th_state, (ArgVal)id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        // perform a dequeue operation
        HQueueApplyDequeue(queue_object, th_state, id);
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
    queue_object = getAlignedMemory(S_CACHE_LINE_SIZE, sizeof(HQueueStruct));
    HQueueInit(queue_object, bench_args.nthreads, bench_args.numa_nodes);

    BarrierSet(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: Enqueue: Object state: %ld\n", queue_object->enqueue_struct->counter);
    fprintf(stderr, "DEBUG: Enqueue: rounds: %d\n", queue_object->enqueue_struct->rounds);
    fprintf(stderr, "DEBUG: Dequeue: Object state: %ld\n", queue_object->dequeue_struct->counter);
    fprintf(stderr, "DEBUG: Dequeue: rounds: %d\n", queue_object->dequeue_struct->rounds);
    volatile Node *first = (Node *)queue_object->first;
    long counter = 0;

    while (first->next != NULL) {
        first = first->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the queue\n", counter);
#endif

    return 0;
}
