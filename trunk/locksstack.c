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
#include "pool.h"


typedef struct ListNode {
    Object value;		      // initially, there is a sentinel node 
    volatile struct ListNode *next; 	        // in the queue where Head and Tail point to.
} ListNode;	

LockStruct *lhead CACHE_ALIGN;
int counter = 0;
ListNode guard CACHE_ALIGN = {0, null};
volatile ListNode *Head CACHE_ALIGN = &guard;
int64_t d1 CACHE_ALIGN, d2;

__thread PoolStruct pool_node;


inline static void push(Object arg, int pid) {
    volatile ListNode *n = alloc_obj(&pool_node);
    n->value = (Object)arg;
    lock(lhead, pid);   // Critical section
    n->next = Head;
    Head = n;
    unlock(lhead, pid);
}



inline static Object pop(int pid) {
    Object result;

    lock(lhead, pid);
    if (Head->next == null) 
        result = -1;
    else {
        result = Head->next->value;
        Head = Head->next;
    }
    unlock(lhead, pid);

    return result;
}

pthread_barrier_t barr;


inline static void Execute(void* Arg) {
    long i;
    long rnum;
    long id = (long) Arg;
    volatile int j;

    _thread_pin(id);
    simSRandom(id + 1);
    init_pool(&pool_node, sizeof(ListNode));
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }

    for (i = 0; i < RUNS; i++) {
        push((Object)1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        pop(id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
}
    

inline static pthread_t StartThread(int arg) {
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

    lhead = lock_init();

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
