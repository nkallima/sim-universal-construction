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

typedef struct ListNode {
    volatile struct ListNode *next;  // in the queue where Head and Tail point to.
    int32_t value;                   // initially, there is a sentinel node 
} ListNode;

CLHLockStruct *lhead, *ltail;
ListNode guard CACHE_ALIGN = {null, 0};

volatile ListNode *Head CACHE_ALIGN = &guard;
volatile ListNode *Tail CACHE_ALIGN = &guard;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;


inline static void enqueue(PoolStruct *pool_node, Object arg, int pid) {
    ListNode *n = alloc_obj(pool_node);

    n->value = (Object)arg;
    n->next = null;
    CLHLock(ltail, pid);
    Tail->next = n;
    Tail = n;
    CLHUnlock(ltail, pid);
}



inline static Object dequeue(int pid) {
    Object result;

    CLHLock(lhead, pid);
    if (Head->next == null) 
        result = -1;
    else {
        result = Head->next->value;
        Head = Head->next;
    }
    CLHUnlock(lhead, pid);

    return result;
}



inline static void *Execute(void* Arg) {
    long i;
    long rnum;
    long id = (long) Arg;
    volatile int j;
    PoolStruct pool_node;

    fastRandomSetSeed(id + 1);
    init_pool(&pool_node, sizeof(ListNode));
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();


    for (i = 0; i < bench_args.runs; i++) {
        enqueue(&pool_node, (Object)1, id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
        dequeue(id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);
    ltail = CLHLockInit(bench_args.nthreads);
    lhead = CLHLockInit(bench_args.nthreads);

    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2 * bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

    return 0;
}
