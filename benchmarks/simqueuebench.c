#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <tvec.h>
#include <fastrand.h>
#include <pool.h>
#include <threadtools.h>
#include <simqueue.h>
#include <barrier.h>
#include <bench_args.h>

SimQueueStruct *queue;
int64_t d1, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

static void *Execute(void *Arg) {
    SimQueueThreadState *th_state;
    long i = 0;
    long id = (long)Arg;
    long rnum;
    volatile int j = 0;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(SimQueueThreadState));
    SimQueueThreadStateInit(queue, th_state, id);

    BarrierWait(&bar);
    if (id == 0) {
        d1 = getTimeMillis();
    }

    for (i = 0; i < bench_args.runs; i++) {
        SimQueueEnqueue(queue, th_state, id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        SimQueueDequeue(queue, th_state, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    queue = getAlignedMemory(CACHE_LINE_SIZE, sizeof(SimQueueStruct));
    SimQueueInit(queue, bench_args.nthreads, bench_args.backoff_high);

    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    Node *link_a = queue->enq_pool[queue->enq_sp.struct_data.index]->link_a;
    Node *link_b = queue->enq_pool[queue->enq_sp.struct_data.index]->link_b;
    CASPTR(&link_a->next, null, link_b);
    fprintf(stderr, "DEBUG: Enqueue: Object state: %ld\n", (long)queue->enq_pool[queue->enq_sp.struct_data.index]->counter);
    fprintf(stderr, "DEBUG: Dequeue: Object state: %ld\n", (long)queue->deq_pool[queue->deq_sp.struct_data.index]->counter);
    volatile Node *cur = queue->deq_pool[queue->deq_sp.struct_data.index]->ptr;
    long counter = 0;
    while (cur != null) {
        cur = cur->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the queue\n", counter - 1); // Do not count queue->guard node
#endif

    return 0;
}
