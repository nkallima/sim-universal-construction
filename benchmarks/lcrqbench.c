#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <barrier.h>
#include <bench_args.h>
#include <lcrq.h>

LCRQStruct *queue_object CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

static void *Execute(void *Arg) {
    LCRQThreadState thread_state;
    long i, rnum;
    volatile int j;
    int id = synchGetThreadId();

    LCRQThreadStateInit(&thread_state, id);
    synchFastRandomSetSeed(id + 1);
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform an enqueue operation
        LCRQEnqueue(queue_object, &thread_state, (ArgVal)id, id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        // perform a dequeue operation
        LCRQDequeue(queue_object, &thread_state, id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    synchBarrierWait(&bar);
    if (id == 0) d2 = synchGetTimeMillis();

#ifdef DEBUG
    synchFAA64(&queue_object->closes, thread_state.mycloses);
    synchFAA64(&queue_object->unsafes, thread_state.myunsafes);
#endif

    return NULL;
}

int main(int argc, char *argv[]) {
    synchParseArguments(&bench_args, argc, argv);
    queue_object = synchGetAlignedMemory(S_CACHE_LINE_SIZE, sizeof(LCRQStruct));
    LCRQInit(queue_object, bench_args.nthreads);

    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);
    d2 = synchGetTimeMillis();

#ifdef DEBUG
    LCRQThreadState thread_state;
    RetVal ret;
    long counter = 0;

    LCRQThreadStateInit(&thread_state, 0);

    // Currently, we don't have a state validation number as in the case of the combining queues.
    // Thus, we simply print the number of threads * number of runs per thread.
    // The actual validation is the number of nodes left at the end of benchmark.
    fprintf(stderr, "DEBUG: Enqueue: Object state: %ld\n", bench_args.nthreads * bench_args.runs);
    fprintf(stderr, "DEBUG: Dequeue: Object state: %ld\n", bench_args.nthreads * bench_args.runs);

    // The actual queue validation code.
    while ((ret = LCRQDequeue(queue_object, &thread_state, 0) != EMPTY_QUEUE)) {
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the queue\n", counter);
    fprintf(stderr, "DEBUG: closes=%ld unsafes=%ld\n", queue_object->closes, queue_object->unsafes);
#endif

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2 * bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);

    return 0;
}