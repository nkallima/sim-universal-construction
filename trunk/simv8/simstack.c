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


int MIN_BAK = 0, MAX_BAK = 0;

typedef struct Node{
    volatile struct Node *next;
    Object value;
} Node;

typedef struct HalfObjectState {
    ToggleVector applied;
    Node *head;
    Object ret[N_THREADS];
#ifdef DEBUG
    int counter;
#endif
} HalfObjectState;


typedef struct ObjectState {
    ToggleVector applied;
    Node *head;
    Object ret[N_THREADS];
#ifdef DEBUG
    int counter;
#endif
    int32_t pad[PAD_CACHE(sizeof(HalfObjectState))];
} ObjectState;


typedef struct pointer_t {
    int64_t seq : 48;
    int32_t index : 16;
} pointer_t;

// Shared variables
volatile int64_t sp CACHE_ALIGN;
volatile ToggleVector a_toggles CACHE_ALIGN;
volatile ArgVal announce[N_THREADS] CACHE_ALIGN;

// Base address of shared memmory
volatile ObjectState pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;

// Each thread owns a private copy of the following variables
__thread BackoffStruct backoff;
__thread ToggleVector mask;
__thread ToggleVector toggle;
__thread ToggleVector my_bit;
__thread int local_index = 0;
__thread PoolStruct pool_node;



inline static void stack_init(volatile ObjectState *st) {
    st->head = null;
}

void SHARED_OBJECT_INIT(int pid) {
    if (pid == 0) {
        pointer_t tmp_sp;

        tmp_sp.index = LOCAL_POOL_SIZE * N_THREADS;
        tmp_sp.seq = 0;
        sp = *((int_fast64_t *)&tmp_sp);
        TVEC_SET_ZERO((ToggleVector *)&a_toggles);

        // OBJECT'S INITIAL VALUE
        // ----------------------
        stack_init(&pool[LOCAL_POOL_SIZE * N_THREADS]);

        TVEC_SET_ZERO((ToggleVector *)&pool[LOCAL_POOL_SIZE * N_THREADS].applied);
#ifdef DEBUG
        pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0;
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

inline static void push(HalfObjectState *st, ArgVal arg, int pid) {
#ifdef DEBUG
    st->counter += 1;
#endif
    Node *n;
    n = alloc_obj(&pool_node);
    n->next = st->head;
    n->value = (ArgVal)arg;
    st->head = n;
}

inline static void pop(HalfObjectState *st, int pid) {
#ifdef DEBUG
    st->counter += 1;
#endif
    if(st->head != null) {
        st->ret[pid] = (RetVal)st->head->value;
        st->head = (Node *)st->head->next;
    }
    else st->ret[pid] = (RetVal)-1;
}

RetVal apply_op(ArgVal arg, int pid) {
    HalfObjectState *lsp, *mod_sp;
    ToggleVector diffs, l_toggles;
    pointer_t ldw, mod_dw;
    int64_t tmp_sp;
    ArgVal tmp_arg;
    int i, j, prefix, mybank, push_counter;

    announce[pid] = arg;                                // announce the operation
	mybank = TVEC_GET_BANK_OF_BIT(pid);
    TVEC_REVERSE_BIT(&my_bit, pid);
    TVEC_NEGATIVE_BANK(&toggle, &toggle, mybank);
    mod_sp = (HalfObjectState *)&pool[pid * LOCAL_POOL_SIZE + local_index];
    TVEC_ATOMIC_ADD_BANK(&a_toggles, &toggle, mybank);  // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
    backoff_play(&backoff);

    for (j = 0; j < 2; j++) {
        tmp_sp = sp;                                   // read reference to struct ObjectState
        ldw = *((pointer_t *)&tmp_sp);
        lsp = (HalfObjectState *)&pool[ldw.index];     // read reference of struct ObjectState in a local variable lsp
        TVEC_ATOMIC_COPY_BANKS(&mod_sp->applied, &lsp->applied, mybank);
        TVEC_AND_BANKS(&diffs, &mod_sp->applied, &mask, mybank);
        diffs = TVEC_XOR(diffs, my_bit);              // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                  // if the operation has already been applied return
            break;
        *mod_sp = *lsp;
        FullFence();
        l_toggles = a_toggles;                      // This is an atomic read, since a_toogles is volatile
        if (tmp_sp != sp)
            continue;
        diffs = TVEC_XOR(mod_sp->applied, l_toggles);
#if N_THREADS > 2
        if (j == 0) 
            backoffCalculate(&backoff, TVEC_COUNT_BITS(diffs));
#endif
        push_counter = 0;
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                tmp_arg = announce[proc_id];
                if (tmp_arg == POP) {
                    pop(mod_sp, proc_id);
                } else {
                    push(mod_sp, tmp_arg, proc_id);
                    push_counter++;
                }
                diffs.cell[i] ^= 1L << pos;
            }
        }

        mod_sp->applied = l_toggles;                             // change applied to be equal to what was read in a_toggles
        mod_dw.seq = ldw.seq + 1;                                // increase timestamp
        mod_dw.index = LOCAL_POOL_SIZE * pid + local_index;      // store in mod_dw.index the index in pool where lsp will be stored
        if (tmp_sp==sp && CAS64(&sp, *((int64_t *)&ldw), *((int64_t *)&mod_dw))) {
            local_index = (local_index + 1) % LOCAL_POOL_SIZE;
            break;
        }
        else rollback(&pool_node, push_counter);
    }
    if (arg == POP)
        return (RetVal)-1;
    LoadFence();
    tmp_sp = sp;                                                 // after two unsuccessful efforts, read current value of sp
    return pool[((pointer_t *)&tmp_sp)->index].ret[pid];         // return the value found in the record stored there
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
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        apply_op((ArgVal) id + 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        apply_op((ArgVal) POP, id);
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
    int i;

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
    fprintf(stderr, "Object state debug counter: %lld\n", (long long int)pool[((pointer_t*)&sp)->index].counter);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
