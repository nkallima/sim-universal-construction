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
#include "thread.h"

#define LOCAL_POOL_SIZE            8

int MIN_BAK = 0, MAX_BAK = 0;


typedef struct STATE{
    Object array[OBJECT_SIZE];
} STATE;

typedef struct HalfObjectState {
    int64_t seq2;
    ToggleVector applied;
    STATE state;
#ifdef DEBUG
    int64_t counter;
    int32_t apps;
#endif
    int64_t seq;
    Object ret[N_THREADS] BIG_ALIGN;
} HalfObjectState;


typedef struct ObjectState {
    int64_t seq2;
    ToggleVector applied;
    STATE state;
#ifdef DEBUG
    int64_t counter;
    int32_t apps;
#endif
    int64_t seq;
    Object ret[N_THREADS];
    int32_t pad[PAD_CACHE(sizeof(HalfObjectState))];
} ObjectState;

typedef struct pointer_t {
    int64_t seq : 48;
    int32_t index : 16;
} pointer_t;

// Shared variables
volatile int_fast64_t sp CACHE_ALIGN;

// Try to place a_toggles and anounce to 
// the same cache line
volatile ToggleVector a_toggles CACHE_ALIGN;
volatile Object announce[N_THREADS] CACHE_ALIGN;

// Base address of shared memmory
volatile ObjectState pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;

// Each thread owns a private copy of the following variables
__thread BackoffStruct backoff;
__thread ToggleVector mask;
__thread ToggleVector toggle;
__thread ToggleVector my_bit;
__thread int_fast32_t local_index = 0;

__thread long long max_latency = 0;

void SHARED_OBJECT_INIT(int pid) {
    pointer_t tmp_sp;
    int i;

    if (pid == 0) {
        tmp_sp.index = LOCAL_POOL_SIZE * N_THREADS;
        tmp_sp.seq = 0L;
        sp = *((int_fast64_t *)&tmp_sp);
        TVEC_SET_ZERO((ToggleVector *)&a_toggles);

        // OBJECT'S INITIAL VALUE
        // ----------------------
        for(i = 0; i < OBJECT_SIZE; i++)
            pool[LOCAL_POOL_SIZE * N_THREADS].state.array[i] = 1;

        pool[LOCAL_POOL_SIZE * N_THREADS].seq = 0L;
        TVEC_SET_ZERO((ToggleVector *)&pool[LOCAL_POOL_SIZE * N_THREADS].applied);
        pool[LOCAL_POOL_SIZE * N_THREADS].seq2 = 0L;
#ifdef DEBUG
        pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0L;
        pool[LOCAL_POOL_SIZE * N_THREADS].apps = 0L;
#endif
        FullFence();
    }

    init_backoff(&backoff, MIN_BAK, MAX_BAK, 1);
    TVEC_SET_ZERO(&mask);
    TVEC_SET_ZERO(&my_bit);
    TVEC_SET_ZERO(&toggle);
    TVEC_REVERSE_BIT(&my_bit, pid);
    TVEC_SET_BIT(&mask, pid);
    toggle = TVEC_NEGATIVE(mask);
}

void fetchAndMultiply(STATE *st, Object arg, int pid) {
    unsigned int j;

    for(j = 0; j < OBJECT_SIZE; j++){
        long tmp = st->array[j];
    	st->array[j] = tmp * ((long) arg);
    }
}



Object apply_op(void (*sfunc)(STATE *, Object, int), Object arg, int pid) {
    ToggleVector diffs, l_toggles;
    pointer_t ldw, mod_dw;
    HalfObjectState *lsp, *mod_sp;
    Object tmp_arg;
    int_fast64_t tmp_sp;
    int i, j;

    announce[pid] = arg;                            // announce the operation
    TVEC_REVERSE_BIT(&my_bit, pid);
    toggle = TVEC_NEGATIVE(toggle);
    mod_sp = (HalfObjectState *)&pool[pid * LOCAL_POOL_SIZE + local_index];
    TVEC_ATOMIC_ADD(&a_toggles, toggle);            // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
    backoff_play(&backoff);

    for (j = 0; j < 2; j++) {
        tmp_sp = sp;                                // read reference to struct ObjectState
        ldw = *((pointer_t *)&tmp_sp);
        lsp = (HalfObjectState *)&pool[ldw.index];  // read reference of struct ObjectState in a local variable lsp
        mod_sp->seq = lsp->seq;
        mod_sp->applied = lsp->applied;
        mod_sp->ret[pid] = lsp->ret[pid];
        mod_sp->seq2 = lsp->seq2;
        if (mod_sp->seq != mod_sp->seq2)            // consistency check
            continue;
        diffs = TVEC_XOR(TVEC_AND(mod_sp->applied, mask), my_bit);   // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                // if the operation has already been applied return
            return mod_sp->ret[pid];
        memcpy(mod_sp, lsp, sizeof (HalfObjectState));
#if N_THREADS > 2
            FullFence();
#endif
        l_toggles = a_toggles;                      // This is an atomic read, since a_toogles is volatile
        if (tmp_sp != sp)
            continue;
        diffs = TVEC_XOR(mod_sp->applied, l_toggles);
#if N_THREADS > 2
        if (j == 0) 
            backoffCalculate(&backoff, TVEC_COUNT_BITS(diffs));
        LoadFence();
        for (i = 0; i < N_THREADS; i += 8)
            __builtin_prefetch((const void *)&announce[i], 0, 3);
#endif
        mod_sp->seq += 1;
        sfunc(&mod_sp->state, arg, pid);
        TVEC_REVERSE_BIT(&diffs, pid);
#ifdef DEBUG
        mod_sp->counter += 1;
        mod_sp->apps += 1;
#endif
        while (TVEC_ACTIVE_THREADS(diffs) != 0L) {               // as long as there are still processes to help
            i = TVEC_SEARCH_FIRST_BIT(diffs);                    // find the next such process
            TVEC_REVERSE_BIT(&diffs, i);                         // extract this process from the se
#ifdef DEBUG
            mod_sp->counter += 1;
#endif
            tmp_arg = announce[i];                               // discover its operation
            sfunc(&mod_sp->state, tmp_arg, i);                   // apply the operation to a local copy of the object's state
        }
        mod_sp->applied = l_toggles;                             // change applied to be equal to what was read in a_toggles
        mod_sp->seq2 += 1;
        mod_dw.seq = ldw.seq + 1;                                // increase timestamp
        mod_dw.index = LOCAL_POOL_SIZE * pid + local_index;      // store in mod_dw.index the index in pool where lsp will be stored
        if (tmp_sp==sp && CAS(&sp, *((int64_t *)&ldw), *((int64_t *)&mod_dw))) { // try to change sp to the value mod_dw
            local_index = (local_index + 1) % LOCAL_POOL_SIZE;   // if this happens successfully,use next item in pid's pool next time
            return mod_sp->ret[pid];
        }
    }
    LoadFence();
    tmp_sp = sp;                                                 // after two unsuccessful efforts, read current value of sp
    return pool[((pointer_t *)&tmp_sp)->index].ret[pid];         // return the value found in the record stored there
}

pthread_barrier_t barr;
int64_t d1 CACHE_ALIGN, d2;

inline static void Execute(void* Arg) {
    long i, rnum;
    long id = (long) Arg;
    volatile long j;

    SHARED_OBJECT_INIT(id);
    _thread_pin(id);
    simSRandom((unsigned long)id + 1L);
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < RUNS; i++) {
        apply_op(fetchAndMultiply, (Object) (id + 1L), id);
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
    fprintf(stderr, "Object state long value: %lld\n", (long long int)pool[((pointer_t*)&sp)->index].counter);
    fprintf(stderr, "Average helping: %f\n", (float)pool[((pointer_t*)&sp)->index].counter/pool[((pointer_t*)&sp)->index].apps);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
