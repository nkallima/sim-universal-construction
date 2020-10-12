#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <barrier.h>

volatile int64_t object CACHE_ALIGN;
pthread_spinlock_t lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar;

void SHARED_OBJECT_INIT(void) {
    object = 1;
}

inline static void *Execute(void* Arg) {
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    fastRandomSetSeed(id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < RUNS; i++) {
        pthread_spin_lock(&lock);
        object = object + 1;
        pthread_spin_unlock(&lock);
        rnum = fastRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(void) {
    SHARED_OBJECT_INIT();
    pthread_spin_init(&lock, PTHREAD_PROCESS_SHARED);
    BarrierInit(&bar, N_THREADS);
    StartThreadsN(N_THREADS, Execute, _DONT_USE_UTHREADS_);
    JoinThreadsN(N_THREADS);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), RUNS*N_THREADS/(1000.0*(d2 - d1)));
    printStats(N_THREADS);

    return 0;
}
