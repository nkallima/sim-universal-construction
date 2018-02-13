#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <sched.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <pool.h>
#include <threadtools.h>
#include <ccqueue.h>
#include <barrier.h>

CCQueueStruct queue_object CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar;

inline static void *Execute(void* Arg) {
    CCQueueThreadState *th_state;
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(CCQueueThreadState));
    CCQueueThreadStateInit(&queue_object, th_state, (int)id);

    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
        // perform an enqueue operation
        CCQueueApplyEnqueue(&queue_object, th_state, (ArgVal) id, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
        // perform a dequeue operation
        CCQueueApplyDequeue(&queue_object, th_state, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(void) {
    CCQueueStructInit(&queue_object);   
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2*RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

#ifdef DEBUG
    fprintf(stderr, "enqueue state:    counter: %d rounds: %d\n", queue_object.enqueue_struct.counter, queue_object.enqueue_struct.rounds);
    fprintf(stderr, "dequeue state:    counter: %d rounds: %d\n\n", queue_object.dequeue_struct.counter, queue_object.dequeue_struct.rounds);
#endif

    return 0;
}
