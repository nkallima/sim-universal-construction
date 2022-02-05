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
#ifdef DEBUG
volatile uint64_t deq_state = 0;
#endif

volatile Node *Tail CACHE_ALIGN = &guard;
#ifdef DEBUG
volatile uint64_t enq_state = 0;
#endif

int64_t d1 CACHE_ALIGN, d2;
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

__thread SynchPoolStruct pool_node;

inline static void enqueue(Object arg, int pid) {
    Node *n = synchAllocObj(&pool_node);

    n->val = (Object)arg;
    n->next = NULL;
    CLHLock(ltail, pid);
    Tail->next = n;
    synchNonTSOFence();
    Tail = n;
#ifdef DEBUG
    enq_state += 1;
#endif
    CLHUnlock(ltail, pid);
}

inline static Object dequeue(int pid) {
    Object result;
    Node *node = NULL;

    CLHLock(lhead, pid);
    if (Head->next == NULL)
        result = EMPTY_QUEUE;
    else {
        node = (Node *)Head;
        Head = Head->next;
        synchNonTSOFence();
        if (node->val == GUARD_VALUE && Head->next != NULL) {
            Head = Head->next;
            result = EMPTY_QUEUE;
        } else {
            result = node->val;
        }
    }
#ifdef DEBUG
    deq_state += 1;
#endif
    CLHUnlock(lhead, pid);
    if (node != NULL) synchRecycleObj(&pool_node, node);

    return result;
}

inline static void *Execute(void *Arg) {
    long i;
    long rnum;
    int id = synchGetThreadId();
    volatile int j;

    synchFastRandomSetSeed(id + 1);
    synchInitPool(&pool_node, sizeof(Node));
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        enqueue((Object)id, id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        dequeue(id);
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
    ltail = CLHLockInit(bench_args.nthreads);
    lhead = CLHLockInit(bench_args.nthreads);

    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), 2 * bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);
#ifdef DEBUG
    volatile Node *head = Head;
    long counter = 0;

    fprintf(stderr, "DEBUG: Enqueue: Object state: %ld\n", enq_state);
    fprintf(stderr, "DEBUG: Dequeue: Object state: %ld\n", deq_state);
    while (head->next != NULL) {
        head = head->next;
        counter++;
    }
    fprintf(stderr, "DEBUG: %ld nodes were left in the queue\n", counter);
#endif

    return 0;
}
