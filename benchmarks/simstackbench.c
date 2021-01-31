#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <simstack.h>
#include <barrier.h>
#include <bench_args.h>

SimStackStruct *stack;
int64_t d1, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    SimStackThreadState *th_state;
    long i = 0;
    long id = (long)Arg;
    long rnum;
    volatile int j = 0;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(SimStackThreadState));
    SimStackThreadStateInit(th_state, bench_args.nthreads, id);
    BarrierWait(&bar);
    if (id == 0) d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        SimStackPush(stack, th_state, id, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        SimStackPop(stack, th_state, id);
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

    stack = getAlignedMemory(CACHE_LINE_SIZE, sizeof(SimStackStruct));
    SimStackInit(stack, bench_args.nthreads, bench_args.backoff_high);
    BarrierSet(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: Object state: %lld\n", (long long int)stack->pool[stack->sp.struct_data.index]->counter);
    volatile Node *head = stack->pool[stack->sp.struct_data.index]->head;
    long counter = 0;
    while (head != NULL) {
        head = head->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the stack\n", counter);
#endif

    return 0;
}
