#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "tvec.h"
#include "rand.h"
#include "backoff.h"
#include "pool.h"
#include "thread.h"

#define GUARD_VALUE                -1

#define LOCAL_POOL_SIZE            16

#define INIT_QUEUE_SIZE            0

int MIN_BAK;
int MAX_BAK;

typedef struct Node {
    Object obj;
    struct Node *next;
} Node;

typedef struct pointer_t {
    int64_t seq : 48;
    int32_t index : 16;
} pointer_t;

typedef struct HalfEnqState {
    ToggleVector applied;
    Node *link_a;
    Node *link_b;
    Node *ptr;
#ifdef DEBUG
    int32_t counter;
#endif
} HalfEnqState;

typedef struct EnqState {
    ToggleVector applied;
    Node *link_a;
    Node *link_b;
    Node *ptr;
#ifdef DEBUG
    int32_t counter;
#endif
    int32_t pad[PAD_CACHE(sizeof (HalfEnqState))];
} EnqState;

typedef struct HalfDeqState {
    ToggleVector applied;
    Node *ptr;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int32_t counter;
#endif
} HalfDeqState;


typedef struct DeqState {
    ToggleVector applied;
    Node *ptr;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int32_t counter;
#endif
    int32_t pad[PAD_CACHE(sizeof (HalfDeqState))];
} DeqState;


// Shared variables
volatile int64_t enq_sp CACHE_ALIGN;
volatile ToggleVector enqueuers CACHE_ALIGN;
ArgVal announce[N_THREADS] CACHE_ALIGN;
EnqState enq_pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;

volatile int64_t deq_sp CACHE_ALIGN;
volatile ToggleVector dequeuers CACHE_ALIGN;
DeqState deq_pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;

// Guard node
// Do not set this as const node
Node guard CACHE_ALIGN = {GUARD_VALUE, null};

// Each thread owns a private copy of the following variables
__thread ToggleVector mask;
__thread ToggleVector deq_toggle;
__thread ToggleVector my_deq_bit;
__thread ToggleVector enq_toggle;
__thread ToggleVector my_enq_bit;
__thread BackoffStruct enq_backoff;
__thread BackoffStruct deq_backoff;
__thread PoolStruct pool_node;
__thread int deq_local_index = 0;
__thread int enq_local_index = 0;


void SHARED_OBJECT_INIT(int pid) {
    if (pid == 0) {
        pointer_t tmp_sp;

        tmp_sp.index = LOCAL_POOL_SIZE * N_THREADS;
        tmp_sp.seq = 0L;
        enq_sp = *((int_fast64_t *) & tmp_sp);

        tmp_sp.index = LOCAL_POOL_SIZE * N_THREADS;
        tmp_sp.seq = 0L;
        deq_sp = *((int_fast64_t *) & tmp_sp);

        TVEC_SET_ZERO((ToggleVector *)&enqueuers);
        TVEC_SET_ZERO((ToggleVector *)&dequeuers);

        // Initializing queue's state
        // --------------------------
        TVEC_SET_ZERO((ToggleVector *) &enq_pool[LOCAL_POOL_SIZE * N_THREADS].applied);
        enq_pool[LOCAL_POOL_SIZE * N_THREADS].link_a = &guard;
        enq_pool[LOCAL_POOL_SIZE * N_THREADS].link_b = null;
        enq_pool[LOCAL_POOL_SIZE * N_THREADS].ptr = &guard;

        TVEC_SET_ZERO((ToggleVector *) &deq_pool[LOCAL_POOL_SIZE * N_THREADS].applied);
        deq_pool[LOCAL_POOL_SIZE * N_THREADS].ptr = &guard;
#ifdef DEBUG
        enq_pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0L;
        deq_pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0L;
#endif
        FullFence();
    }

    TVEC_SET_ZERO(&mask);
    TVEC_SET_ZERO(&my_enq_bit);
    TVEC_SET_ZERO(&enq_toggle);
    TVEC_REVERSE_BIT(&my_enq_bit, pid);
    TVEC_SET_BIT(&mask, pid);
    enq_toggle = TVEC_NEGATIVE(mask);
    init_backoff(&enq_backoff, MIN_BAK, MAX_BAK, 1);
    init_pool(&pool_node, sizeof(Node));

    TVEC_SET_ZERO(&mask);
    TVEC_SET_ZERO(&my_deq_bit);
    TVEC_SET_ZERO(&deq_toggle);
    TVEC_REVERSE_BIT(&my_deq_bit, pid);
    TVEC_SET_BIT(&mask, pid);
    deq_toggle = TVEC_NEGATIVE(mask);
    init_backoff(&deq_backoff, MIN_BAK, MAX_BAK, 1);
}


inline static void connectQueue(void) {
    EnqState lsp;
    int_fast64_t tmp_sp;

    LoadFence();
    tmp_sp = enq_sp;
    memcpy(&lsp, &enq_pool[ ((pointer_t *) & tmp_sp)->index], sizeof(HalfEnqState));
    CASPTR(&lsp.link_a->next, null, lsp.link_b);
}

void enqueue(ArgVal arg, int pid) {
    ToggleVector diffs, l_toggles;
    pointer_t ldw, mod_dw;
    int64_t tmp_sp, enq_active;
    int i, j, enq_counter, prefix, mybank;
    EnqState *mod_sp, *lsp;
    Node *node, *llist;
	
    announce[pid] = arg; // A Fetch&Add instruction follows soon, thus a barrier is needless
	mybank = TVEC_GET_BANK_OF_BIT(pid);
    TVEC_REVERSE_BIT(&my_enq_bit, pid);
    TVEC_NEGATIVE_BANK(&enq_toggle, &enq_toggle, mybank);
    mod_sp = &enq_pool[pid * LOCAL_POOL_SIZE + enq_local_index];
    TVEC_ATOMIC_ADD_BANK(&enqueuers, &enq_toggle, mybank);            // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
    backoff_play(&enq_backoff);

    for (j = 0; j < 2; j++) {
        tmp_sp = enq_sp;          // This is an atomic read, since sp is volatile
        ldw = *((pointer_t *) & tmp_sp);
        lsp = &enq_pool[ldw.index];
        TVEC_ATOMIC_COPY_BANKS(&mod_sp->applied, &lsp->applied, mybank);
        TVEC_AND_BANKS(&diffs, &mod_sp->applied, &mask, mybank);
        diffs = TVEC_XOR(diffs, my_enq_bit);   // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                // if the operation has already been applied return
            return;
        *mod_sp = *lsp;
        FullFence();
        l_toggles = enqueuers;    // This is an atomic read, since sp is volatile
        if (tmp_sp != enq_sp)
            continue;
        diffs = TVEC_XOR(mod_sp->applied, l_toggles);
        if (mod_sp->link_a->next == null)  // avoid owned state (MOESI protocol)
            CASPTR(&mod_sp->link_a->next, null, mod_sp->link_b);
        if (j == 0)
            backoffCalculate(&enq_backoff, TVEC_COUNT_BITS(diffs));
        enq_counter = 1;
        node = alloc_obj(&pool_node);
        node->next = null;
        node->obj = arg;
        llist = node;
        TVEC_REVERSE_BIT(&diffs, pid);
#ifdef DEBUG
        mod_sp->counter += 1;
#endif
        enq_counter = 0;
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                enq_counter++;
#ifdef DEBUG
                mod_sp->counter += 1;
#endif
				node->next = alloc_obj(&pool_node);
                node = node->next;
                node->next = null;
                node->obj = announce[proc_id];
                diffs.cell[i] ^= 1L << pos;
            }
        }

        mod_sp->link_a = mod_sp->ptr;
        mod_sp->link_b = llist;
        mod_sp->ptr = node;
        mod_sp->applied = l_toggles;
        mod_dw.seq = ldw.seq + 1;
        mod_dw.index = LOCAL_POOL_SIZE * pid + enq_local_index;
        if (tmp_sp == enq_sp && CAS64(&enq_sp, tmp_sp, *((int64_t *)&mod_dw))) {
            CASPTR(&mod_sp->link_a->next, null, mod_sp->link_b);
            enq_local_index = (enq_local_index + 1) % LOCAL_POOL_SIZE;
            return;
        }
        else rollback(&pool_node, enq_counter);
    }
    return;
}


RetVal dequeue(int pid) {
    ToggleVector diffs, l_toggles;
    int64_t tmp_sp;
    int i, j, prefix, mybank;
    DeqState *mod_sp, *lsp;
    pointer_t ldw, mod_dw;
    Node *next;

	mybank = TVEC_GET_BANK_OF_BIT(pid);
    TVEC_REVERSE_BIT(&my_deq_bit, pid);
    TVEC_NEGATIVE_BANK(&deq_toggle, &deq_toggle, mybank);
    mod_sp = &deq_pool[pid * LOCAL_POOL_SIZE + deq_local_index];
    TVEC_ATOMIC_ADD_BANK(&dequeuers, &deq_toggle, mybank);            // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
    backoff_play(&deq_backoff);

    for (j = 0; j < 2; j++) {
        tmp_sp = deq_sp;          // This is an atomic read, since sp is volatile
        ldw = *((pointer_t *) & tmp_sp);
        lsp = &deq_pool[ldw.index];
        TVEC_ATOMIC_COPY_BANKS(&mod_sp->applied, &lsp->applied, mybank);
        TVEC_AND_BANKS(&diffs, &mod_sp->applied, &mask, mybank);
        diffs = TVEC_XOR(diffs, my_deq_bit);   // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                // if the operation has already been applied return
            return;
        *mod_sp = *lsp;
        FullFence();
        l_toggles = dequeuers; // This is an atomic read, since sp is volatile
        if (tmp_sp != deq_sp)
            continue;
        diffs = TVEC_XOR(mod_sp->applied, l_toggles);
        if (j == 0)
            backoffCalculate(&deq_backoff, TVEC_COUNT_BITS(diffs));
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                next = mod_sp->ptr->next;
#ifdef DEBUG
                mod_sp->counter += 1;
#endif
                if (next != null) {
                    mod_sp->ret[i] = next->obj;
                    mod_sp->ptr = next;
                } else {
                    connectQueue();
                    next = mod_sp->ptr->next;
                    if (next == null)
                        mod_sp->ret[proc_id] = GUARD_VALUE;
                    else {
                        mod_sp->ret[proc_id] = next->obj;
                        mod_sp->ptr = next;
                    }
                }
                diffs.cell[i] ^= 1L << pos;
            }
        }
        mod_sp->applied = l_toggles;
        mod_dw.seq = ldw.seq + 1;
        mod_dw.index = LOCAL_POOL_SIZE * pid + deq_local_index;
        if (tmp_sp == deq_sp && CAS64(&deq_sp, *((int64_t *)&ldw), *((int64_t *)&mod_dw))) {
            deq_local_index = (deq_local_index + 1) % LOCAL_POOL_SIZE;
            return mod_sp->ret[pid];
        }
    }

    LoadFence();
    tmp_sp = deq_sp;
    return deq_pool[((pointer_t *)&tmp_sp)->index].ret[pid];
}


pthread_barrier_t barr;
int64_t d1 CACHE_ALIGN, d2;

inline void Execute(void* Arg) {
    long i = 0;
    long id = (long) Arg;
    long rnum;
    volatile int j = 0;

    // Synchronization point
    simSRandom(id + 1);
    SHARED_OBJECT_INIT(id);
    if (id == 0) {
        for (i = 0; i < INIT_QUEUE_SIZE; i++)
             enqueue(id, id);
        d1 = getTimeMillis();
    }
    long rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        enqueue(id, id);
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

int main(int argc, char *argv[]) {
    pthread_t threads[N_THREADS];
    long i;

    init_cpu_counters();
    if (argc != 3) {
        fprintf(stderr, "Please set upper and lower bound for backoff!\n");
        exit(EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &MIN_BAK);
        sscanf(argv[2], "%d", &MAX_BAK);
    }

    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    printf("time: %d\t", (int) (d2 - d1));
    printStats();

#ifdef DEBUG
    Node *link_a = enq_pool[((pointer_t*) & enq_sp)->index].link_a;
    Node *link_b = enq_pool[((pointer_t*) & enq_sp)->index].link_b;
    CASPTR(&link_a->next, null, link_b);
    fprintf(stderr, "Object state value: %ld -- ", (long)enq_pool[((pointer_t*) & enq_sp)->index].counter);
    Node *cur = deq_pool[((pointer_t*) & deq_sp)->index].ptr;
    long counter = 0;
    while (cur != null) {
        cur = cur->next;
        counter++;
    }
    fprintf(stderr, "%ld nodes were left in the queue.\n", counter - 1); // Do not count guard node 
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
