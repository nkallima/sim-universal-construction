#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <backoff.h>
#include <fastrand.h>
#include <threadtools.h>
#include <msqueue.h>
#include <barrier.h>

MSQueue queue CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
int MIN_BAK, MAX_BAK;
Barrier bar;

inline static void *Execute(void* Arg) {
    MSQueueThreadState *th_state;
    long i;
    long id = (long) Arg;
    long rnum;
    volatile long j;

    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(MSQueueThreadState));
    MSQueueThreadStateInit(th_state, MIN_BAK, MAX_BAK);
    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
        MSQueueEnqueue(&queue, th_state, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        MSQueueDequeue(&queue, th_state);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Please set upper and lower bound for backoff!\n");
        exit(EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &MIN_BAK);
        sscanf(argv[2], "%d", &MAX_BAK);
    }

    MSQueueInit(&queue);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2*RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);
#ifdef DEBUG
    int counter = 0;

    while(queue.head != null) {
        queue.head = queue.head->next;
        counter++;
    }
    fprintf(stderr, "%d nodes were left in the queue!\n", counter - 1);
#endif

    return 0;
}
