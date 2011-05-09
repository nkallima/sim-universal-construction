#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "clh.h"
#include "thread.h"

typedef struct STATE{
    long array[OBJECT_SIZE];
} STATE;


typedef struct ObjectState {
    STATE state;
} ObjectState;

volatile ObjectState sp CACHE_ALIGN;
LockStruct *object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;


void SHARED_OBJECT_INIT(void) {
    int j;

    for(j = 0; j < OBJECT_SIZE; j++) {
        sp.state.array[j] = 1L;
    }

    object_lock = lock_init();   
}


long fetchAndMultiply(ObjectState *sp, Object arg, int pid) {
	long j;

        for(j = 0; j < OBJECT_SIZE; j++) {
             long tmp = sp->state.array[j];
             sp->state.array[j] = tmp * ((long) arg);
        }
	return -1;
}

void apply_op(long (*sfunc)(ObjectState *, Object, int), Object arg, int pid) {
	lock(object_lock, pid);
	sfunc((ObjectState *)&sp, arg, pid);
	unlock(object_lock, pid);
}

// Global Variables
pthread_barrier_t barr;


inline void Execute(void* Arg) {
    long i, rnum;
    volatile long j;
    long id = (long) Arg;

    simSRandom(id + 1L);
    if (id == 0)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }

    for (i = 0; i < RUNS; i++) {
        apply_op(fetchAndMultiply, (Object) i, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
    }
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
}

inline pthread_t StartThread(int arg) {
    long id = (long) arg;
    void *Arg = (void*) id;
    pthread_t thread_p;
    int thread_id;

    pthread_attr_t my_attr;
    pthread_attr_init(&my_attr);
    thread_id = pthread_create(&thread_p, &my_attr, EntryPoint, Arg);

    return thread_p;
}

int main(void) {
    pthread_t threads[N_THREADS];
    int i;

    // Barrier initialization
    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    SHARED_OBJECT_INIT();
    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    printf("%d\n", (int) (d2 - d1));

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
