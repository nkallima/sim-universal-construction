#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <queue-stack.h>
#include <primitives.h>
#include <fastrand.h>
#include <clh.h>
#include <threadtools.h>
#include <pool.h>
#include <barrier.h>
#include <bench_args.h>


CLHLockStruct *lhead, *ltail;
Node guard CACHE_ALIGN = {GUARD_VALUE, NULL};

volatile Node *Head CACHE_ALIGN = &guard;
volatile Node *Tail CACHE_ALIGN = &guard;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;

__thread PoolStruct pool_node;

inline static void enqueue(Object arg, int pid) {
    Node *n = alloc_obj(&pool_node);

    n->val = (Object)arg;
    n->next = null;
    CLHLock(ltail, pid);
    Tail->next = n;
    Tail = n;
    CLHUnlock(ltail, pid);
}

inline static Object dequeue(int pid) {
    Object result;
    Node *n = NULL;

    CLHLock(lhead, pid);
    if (Head->next == null)
        return -1;
    else {
        result = Head->val;
        n = (Node *)Head;
        Head = Head->next;
    }
    CLHUnlock(lhead, pid);
    recycle_obj(&pool_node, n);

    return result;
}

inline static void *Execute(void *Arg) {
    long i;
    long rnum;
    long id = (long)Arg;
    volatile int j;

    fastRandomSetSeed(id + 1);
    init_pool(&pool_node, sizeof(Node));
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        enqueue((Object)1, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        dequeue(id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    BarrierWait(&bar);
    if (id == 0) d2 = getTimeMillis();
    destroy_pool(&pool_node);

    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    ltail = CLHLockInit(bench_args.nthreads);
    lhead = CLHLockInit(bench_args.nthreads);

    BarrierSet(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    printStats(bench_args.nthreads, bench_args.total_runs);
#ifdef DEBUG
    volatile Node *head = Head;
    long counter = 0;

    while (head->next != null) {
        head = head->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the queue\n", counter);
#endif

    return 0;
}
