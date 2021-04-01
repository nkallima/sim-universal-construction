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
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;


inline static void *Execute(void *Arg) {
    LCRQThreadState thread_state;
    long i, rnum;
    volatile int j;
    long id = (long)Arg;

    LCRQThreadStateInit(&thread_state, id);
    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform an enqueue operation
        LCRQEnqueue(queue_object, &thread_state, (ArgVal)id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        // perform a dequeue operation
        LCRQDequeue(queue_object, &thread_state, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    BarrierWait(&bar);
    if (id == 0) d2 = getTimeMillis();

#ifdef DEBUG
    FAA64(&queue_object->closes, thread_state.mycloses);
    FAA64(&queue_object->unsafes, thread_state.myunsafes);
#endif

    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    queue_object = getAlignedMemory(S_CACHE_LINE_SIZE, sizeof(LCRQStruct));
    LCRQInit(queue_object, bench_args.nthreads);

    BarrierSet(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

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
    printStats(bench_args.nthreads, bench_args.total_runs);

    return 0;
}