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

#define POOL_SIZE                  1024

CLHLockStruct *lhead, *ltail;


typedef struct ListNode {
    volatile struct ListNode *next; 	        // in the queue where Head and Tail point to.
    int32_t value;		      // initially, there is a sentinel node 
} ListNode;	

ListNode guard CACHE_ALIGN = {null, 0};

volatile ListNode *Head CACHE_ALIGN = &guard;
volatile ListNode *Tail CACHE_ALIGN = &guard;
int64_t d1 CACHE_ALIGN, d2;


inline static void enqueue(PoolStruct *pool_node, Object arg, int pid) {
    ListNode *n = alloc_obj(pool_node);

    n->value = (Object)arg;
    n->next = null;
    clhLock(ltail, pid);
    Tail->next = n;
    Tail = n;
    clhUnlock(ltail, pid);
}



inline static Object dequeue(int pid) {
    Object result;

    clhLock(lhead, pid);
    if (Head->next == null) 
        result = -1;
    else {
        result = Head->next->value;
        Head = Head->next;
    }
    clhUnlock(lhead, pid);

    return result;
}

pthread_barrier_t barr;


inline void Execute(void* Arg) {
    long i;
    long rnum;
    long id = (long) Arg;
    volatile int j;
	PoolStruct pool_node;

    setThreadId(id);
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
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        enqueue(&pool_node, (Object)1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        dequeue(id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    stop_cpu_counters(id);
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

    init_cpu_counters();
   // Barrier initialization
    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    ltail = clhLockInit();
    lhead = clhLockInit();

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
