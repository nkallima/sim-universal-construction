#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <clh.h>
#include <threadtools.h>
#include <pool.h>
#include <barrier.h>
#include <bench_args.h>
#include <queue-stack.h>

CLHLockStruct *lock CACHE_ALIGN;
Node guard CACHE_ALIGN = {0, NULL};

volatile Node *Top CACHE_ALIGN = &guard;
#ifdef DEBUG
volatile uint64_t stack_state = 0;
#endif

int64_t d1 CACHE_ALIGN, d2;
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

__thread SynchPoolStruct pool_node;

inline static void push(Object arg, int pid) {
    volatile Node *n = synchAllocObj(&pool_node);
    n->val = (Object)arg;
    CLHLock(lock, pid);  // Critical section
    n->next = Top;
    Top = n;
#ifdef DEBUG
    stack_state += 1;
#endif
    synchNonTSOFence();
    CLHUnlock(lock, pid);
}

inline static Object pop(int pid) {
    Object result;
    Node *n = NULL;

    CLHLock(lock, pid);
    if (Top->next == NULL)
        result = EMPTY_STACK;
    else {
        result = Top->val;
        n = (Node *)Top;
        Top = Top->next;
    }
#ifdef DEBUG
    stack_state += 1;
#endif
    synchNonTSOFence();
    CLHUnlock(lock, pid);
    if (n != NULL) synchRecycleObj(&pool_node, n);

    return result;
}

inline static void *Execute(void *Arg) {
    long i;
    long rnum;
    long id = (long)Arg;
    volatile int j;

    synchFastRandomSetSeed(id + 1);
    synchInitPool(&pool_node, sizeof(Node));
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        push((Object)id, id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        pop(id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    synchBarrierWait(&bar);
    if (id == 0) d2 = synchGetTimeMillis();
#ifndef DEBUG
    synchDestroyPool(&pool_node);
#endif

    return NULL;
}

int main(int argc, char *argv[]) {
    synchParseArguments(&bench_args, argc, argv);
    lock = CLHLockInit(bench_args.nthreads);

    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    volatile Node *ltop = Top;
    long counter = 0;

    fprintf(stderr, "DEBUG: Object state: %ld\n", stack_state);
    while (ltop->next != NULL) {
        ltop = ltop->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes left in the queue\n", counter);
#endif

    return 0;
}
