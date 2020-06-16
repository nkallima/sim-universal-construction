#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <tvec.h>
#include <fastrand.h>
#include <pool.h>
#include <threadtools.h>
#include <simstack.h>
#include <barrier.h>

SimStackStruct *stack;
int64_t d1, d2;
Barrier bar;

inline static void *Execute(void* Arg) {
    SimStackThreadState *th_state;
    long i = 0;
    long id = (long) Arg;
    long rnum;
    volatile int j = 0;

    fastRandomSetSeed(id + 1);
    th_state = getAlignedMemory(CACHE_LINE_SIZE, sizeof(SimStackThreadState));
    SimStackThreadStateInit(th_state, id);
    BarrierWait(&bar);
    if (id == 0) {
        d1 = getTimeMillis();
    }

    for (i = 0; i < RUNS; i++) {
        SimStackPush(stack, th_state, id, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        SimStackPop(stack, th_state, id);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int backoff;

    if (argc < 2) {
        fprintf(stderr, "ERROR: Please set an upper bound for the backoff!\n");
        exit(EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &backoff);
    }
    stack = getAlignedMemory(CACHE_LINE_SIZE, sizeof(SimStackStruct));
    SimStackInit(stack, backoff);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2*RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

#ifdef DEBUG
    fprintf(stderr, "Object state debug counter: %lld\n", (long long int)stack->pool[stack->sp.struct_data.index].counter);
#endif

    return 0;
}
