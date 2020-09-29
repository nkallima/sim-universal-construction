#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <lfstack.h>
#include <barrier.h>

LFStack stack CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
int MIN_BAK, MAX_BAK;
Barrier bar;

inline static void *Execute(void* Arg) {
    LFStackThreadState *th_state;
    long i;
    long id = (long) Arg;
    long rnum;
    volatile long j;

    setThreadId(id);
    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof (LFStackThreadState));
    LFStackThreadStateInit(th_state, MIN_BAK, MAX_BAK);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
        LFStackPush(&stack, th_state, id + 1);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        LFStackPop(&stack, th_state);
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

    LFStackInit(&stack);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2 * RUNS * N_THREADS / (1000.0 * (d2 - d1)));
    printStats(N_THREADS);
#ifdef DEBUG
    int counter = 0;

    while (stack.top != null) {
        counter++;
        stack.top = stack.top->next;
    }

    fprintf(stderr, "DEBUG: %d nodes were left in the stack!\n", counter);
#endif

    return 0;
}
