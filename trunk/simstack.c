#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "tvec.h"
#include "backoff.h"
#include "pool.h"
#include "thread.h"

#define LOCAL_POOL_SIZE            16

#define POP                        0

int MIN_BAK;
int MAX_BAK;

typedef struct Node{
    struct Node *next;
    Object value;
} Node;

typedef struct HalfObjectState {
    int64_t seq;
    ToggleVector applied;
    Node *head;
#ifdef DEBUG
    int64_t counter;
#endif
    int64_t seq2;
    RetVal ret[N_THREADS];
} HalfObjectState;


typedef struct ObjectState {
    int64_t seq;
    ToggleVector applied;
    Node *head;
#ifdef DEBUG
    int64_t counter;
#endif
    int64_t seq2;
    RetVal ret[N_THREADS];
    int32_t pad[PAD_CACHE(sizeof(HalfObjectState))];
} ObjectState;

typedef struct pointer_t {
    int64_t seq : 48;
    int32_t index : 16;
} pointer_t;

// Shared variables
volatile int_fast64_t sp CACHE_ALIGN;
volatile ToggleVector a_toggles CACHE_ALIGN;
volatile ArgVal announce[N_THREADS] CACHE_ALIGN;

// Base address of shared memmory
volatile ObjectState pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;

// Each thread owns a private copy of the following variables
__thread ToggleVector mask;
__thread ToggleVector toggle;
__thread ToggleVector my_bit;
__thread int local_index = 0;

__thread PoolStruct pool_node;
__thread BackoffStruct backoff;


inline static void stack_init(volatile ObjectState *st) {
    st->head = null;
}

void SHARED_OBJECT_INIT(int pid) {
    if (pid == 0) {
        pointer_t tmp_sp;

        tmp_sp.index = LOCAL_POOL_SIZE * N_THREADS;
        tmp_sp.seq = 0L;
        sp = *((int_fast64_t *)&tmp_sp);
        TVEC_SET_ZERO((ToggleVector *)&a_toggles);

        // OBJECT'S INITIAL VALUE
        // ----------------------
        stack_init(&pool[LOCAL_POOL_SIZE * N_THREADS]);

        pool[LOCAL_POOL_SIZE * N_THREADS].seq = 0L;
        TVEC_SET_ZERO((ToggleVector *)&pool[LOCAL_POOL_SIZE * N_THREADS].applied);
        pool[LOCAL_POOL_SIZE * N_THREADS].seq2 = 0L;
#ifdef DEBUG
        pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0L;
#endif
        FullFence();
    }

    TVEC_SET_ZERO(&mask);
    TVEC_SET_ZERO(&my_bit);
    TVEC_SET_ZERO(&toggle);
    TVEC_REVERSE_BIT(&my_bit, pid);
    TVEC_SET_BIT(&mask, pid);
    toggle = TVEC_NEGATIVE(mask);

    init_backoff(&backoff, MIN_BAK, MAX_BAK, 1);
    init_pool(&pool_node, sizeof(Node));
}

inline static void push_pop(ObjectState *st, ArgVal arg, int pid) {
    if (arg == POP) {       // Executing a pop operation
        if(st->head != null) {
            st->ret[pid] = (RetVal)st->head->value;
            st->head = st->head->next;
        }
        else st->ret[pid] = (RetVal)-1;
    } else {                // Executing a push operation
        Node *n;
        n = alloc_obj(&pool_node);
        n->next = st->head;
        n->value = (ArgVal)arg;
        st->head = n;
    }
}


RetVal apply_op(void (*sfunc)(ObjectState *, ArgVal, int), ArgVal arg, int pid) {
    ObjectState *lsp, *mod_sp;
    ToggleVector diffs, l_toggles, pops;
    pointer_t ldw, mod_dw;
    ArgVal tmp_arg;
    int_fast64_t tmp_sp;
    int i, j, counter;

    TVEC_SET_ZERO(&pops);
    TVEC_REVERSE_BIT(&my_bit, pid);
    toggle = TVEC_NEGATIVE(toggle);
    mod_sp = (ObjectState *)&pool[LOCAL_POOL_SIZE * pid + local_index];
    announce[pid] = arg;
    TVEC_ATOMIC_ADD(&a_toggles, toggle);
    backoff_play(&backoff);

    for (j = 0; j < 2; j++) {
        LoadFence();
        tmp_sp = sp;          // This is an atomic read, since sp is volatile
        ldw = *((pointer_t *)&tmp_sp);
        lsp = (ObjectState *)&pool[ldw.index];
        mod_sp->seq = lsp->seq;
        mod_sp->applied = lsp->applied;
        mod_sp->ret[pid] = lsp->ret[pid];
        mod_sp->seq2 = lsp->seq2;
        if (mod_sp->seq != mod_sp->seq2)
            continue;
        diffs = TVEC_XOR(TVEC_AND(mod_sp->applied, mask), my_bit);
        if (TVEC_IS_SET(diffs, pid))
            return mod_sp->ret[pid];
        memcpy(mod_sp, lsp, sizeof (HalfObjectState));
        if (TVEC_COUNT_BITS(mod_sp->applied) > 2)
            FullFence();
        l_toggles = a_toggles;     // This is an atomic read, since a_toogles is volatile
        if (tmp_sp != sp)
            continue;
        diffs = TVEC_XOR(mod_sp->applied, l_toggles);
        if (j == 1)
            backoffCalculate(&backoff, TVEC_COUNT_BITS(diffs));
        for (i = 0; i < N_THREADS; i += 8)
            ReadPrefetch(&announce[i]);
        mod_sp->seq += 1;
        sfunc(mod_sp, arg, pid);
        TVEC_REVERSE_BIT(&diffs, pid);
#ifdef DEBUG
        mod_sp->counter += 1;
#endif
        counter = 0;
        while (TVEC_ACTIVE_THREADS(diffs) != 0L) {
            i = TVEC_SEARCH_FIRST_BIT(diffs);
            TVEC_REVERSE_BIT(&diffs, i);
#ifdef DEBUG
            mod_sp->counter += 1;
#endif
            tmp_arg = *((volatile ArgVal *)&announce[i]);
            if (tmp_arg != POP) {
                sfunc(mod_sp, tmp_arg, i);
                counter++;
            }
            else TVEC_SET_BIT(&pops, i);
        }

        while (TVEC_ACTIVE_THREADS(pops) != 0L) {
            i = TVEC_SEARCH_FIRST_BIT(pops);
            TVEC_REVERSE_BIT(&pops, i);
            sfunc(mod_sp, POP, i);
        }
        mod_sp->applied = l_toggles;
        mod_sp->seq2 += 1;
        mod_dw.seq = ldw.seq + 1;
        mod_dw.index = LOCAL_POOL_SIZE * pid + local_index;
        if (tmp_sp != sp) {
            rollback(&pool_node, counter);
            continue;
        }
        if (CAS(&sp, *((int64_t *)&ldw), *((int64_t *)&mod_dw))) {
            local_index = (local_index + 1) % LOCAL_POOL_SIZE;
            return mod_sp->ret[pid];
        }
        else rollback(&pool_node, counter);
    }
    if (arg == POP)
        return (RetVal)-1;
    else {
        LoadFence();
        tmp_sp = sp;
        return pool[((pointer_t *)&tmp_sp)->index].ret[pid];
    }
}

pthread_barrier_t barr;
int64_t d1 CACHE_ALIGN, d2;


inline void Execute(void* Arg) {
    long i;
    long id = (long) Arg;
    long rnum;
    volatile long j;

    simSRandom(id + 1);
    SHARED_OBJECT_INIT(id);
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < RUNS; i++) {
        apply_op(push_pop, (ArgVal) id + 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        apply_op(push_pop, (ArgVal) POP, id);
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

    printf("%d\n", (int) (d2 - d1));
    
#ifdef DEBUG
    fprintf(stderr, "Object state debug counter: %lld\n", (long long int)pool[((pointer_t*)&sp)->index].counter);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
