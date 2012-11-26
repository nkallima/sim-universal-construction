#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>
#include <sched.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "ccsynch.h"
#include "pool.h"
#include "thread.h"

#define  GUARD          INT_MIN


typedef struct ListNode {
    Object val;
    struct ListNode *next;
} ListNode;

ListNode guard_node CACHE_ALIGN = {GUARD, null};

volatile ListNode *last CACHE_ALIGN = &guard_node;
volatile ListNode *first CACHE_ALIGN = &guard_node;
int64_t d1 CACHE_ALIGN, d2;


LockStruct enqueue_lock CACHE_ALIGN;
LockStruct dequeue_lock CACHE_ALIGN;

__thread ThreadState lenqueue_lock;
__thread ThreadState ldequeue_lock;
__thread PoolStruct pool_node;

inline static RetVal enqueue(ArgVal arg, int pid) {
     ListNode *node = alloc_obj(&pool_node);
     node->next = null;
     node->val = arg;
     last->next = node;
     last = node;
     return -1;
}

inline static RetVal dequeue(ArgVal arg, int pid) {
     ListNode *node = (ListNode *)first;
	 if (first->next != null){
        first = first->next;
        return node->val;
	 } 
	 else {
         return -1;
     }
}

pthread_barrier_t barr;


inline void Execute(void* Arg) {
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    _thread_pin(id);
    simSRandom(id + 1);
    threadStateInit(&lenqueue_lock, &enqueue_lock, (int)id);
    threadStateInit(&ldequeue_lock, &dequeue_lock, (int)id);
    init_pool(&pool_node, sizeof(ListNode));

    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        // perform an enqueue operation
        applyOp(&enqueue_lock, &lenqueue_lock, enqueue, (ArgVal) 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
        // perform a dequeue operation
        applyOp(&dequeue_lock, &ldequeue_lock, dequeue, (ArgVal) 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
    }
    stop_cpu_counters(id);
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return NULL;
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

    init_cpu_counters();
    // Barrier initialization
    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    lock_init(&enqueue_lock);   
    lock_init(&dequeue_lock); 

    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    printf("time: %d\t", (int) (d2 - d1));
    printStats();

#ifdef DEBUG
    fprintf(stderr, "enqueue state:    counter: %d rounds: %d\n", enqueue_lock.counter, enqueue_lock.rounds);
    fprintf(stderr, "dequeue state:    counter: %d rounds: %d\n\n", dequeue_lock.counter, dequeue_lock.rounds);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
