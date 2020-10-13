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
#include <oscistack.h>
#include <barrier.h>
#include <bench_args.h>

OsciStackStruct object_struct CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void* Arg) {
    OsciStackThreadState *th_state;
    long i, rnum;
    volatile int j;
    long pid = (long)Arg;

    fastRandomSetSeed(pid);
    BarrierWait(&bar);
    if (pid == 0) {
        d1 = getTimeMillis();
    }
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(OsciStackThreadState));
    OsciStackThreadStateInit(&object_struct, th_state, pid);
    for (i = 0; i < bench_args.runs; i++) {
        // perform a push operation
        OsciStackApplyPush(&object_struct, th_state, pid, pid);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        // perform a pop operation
        OsciStackApplyPop(&object_struct, th_state, pid);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);

    OsciStackInit(&object_struct, bench_args.nthreads, bench_args.fibers_per_thread);
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2 * bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

#ifdef DEBUG
    volatile Node *head = object_struct.head;
    int i;

    i = 0;
    while (head != null) {
        head = head->next;
        i++;
    }
    fprintf(stderr, "DEBUG: object state: counter: %d rounds: %d nodes left in the queue: %d\n", object_struct.object_struct.counter, object_struct.object_struct.rounds, i);
#endif

    return 0;
}
