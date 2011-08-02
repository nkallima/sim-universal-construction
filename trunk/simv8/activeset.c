#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "tvec.h"
#include "rand.h"
#include "backoff.h"
#include "thread.h"


pthread_barrier_t barr;
int64_t d1 CACHE_ALIGN, d2;
volatile ToggleVector active_set;

volatile Object announce[N_THREADS];

void SHARED_OBJECT_INIT(void) {
    int i;

    for (i = 0; i < N_THREADS; i++)
        announce[i] = 0;
    TVEC_SET_ZERO((ToggleVector *)&active_set);
}


inline static void Execute(void* Arg) {
    long i, rnum, k, prefix, mybank;
	volatile long j;
    long id = (long) Arg, proc_id;
	Object lannounce[N_THREADS];
    ToggleVector lactive_set;
    ToggleVector mystate;

    simSRandom((unsigned long)id + 1L);
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }
    TVEC_SET_ZERO(&mystate);
	TVEC_SET_BIT(&mystate, id);
    mybank = TVEC_GET_BANK_OF_BIT(id);
    for (i = 0; i < RUNS; i++) {
        announce[id] = i;
        TVEC_ATOMIC_COPY_BANKS(&active_set, &mystate, mybank);   // this function executes join, leave instructions
                                                                 // in the active set object
        mystate = TVEC_NEGATIVE(mystate);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        // Just a simple read of the contents of the active set
        // data implements the GetSet operation of an active set
        lactive_set = active_set;

		for (k = 0, prefix = 0; k < _TVEC_CELLS_; k++){
            prefix  += _TVEC_BIWORD_SIZE_;
            while (lactive_set.cell[k] != 0L) {
			    register int pos, proc_id;

				pos = bitSearchFirst(lactive_set.cell[k]);
				proc_id = prefix + pos;
				lactive_set.cell[k] ^= 1L << pos;
				lannounce[proc_id] = announce[proc_id];
			}
		}
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

int main(int argc, char *argv[]) {
    pthread_t threads[N_THREADS];
    int i;


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

    printf("time: %d\t", (int) (d2 - d1));
    printStats();

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
