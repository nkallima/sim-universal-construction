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

#define LOCAL_POOL_SIZE            16

int MIN_BAK = 0, MAX_BAK = 0;

typedef union CRStruct {
    volatile Object obj;
    char pad[CACHE_LINE_SIZE];
} CRStruct;

typedef struct STATE{
    CRStruct Critical[OBJECT_SIZE] CACHE_ALIGN;
} STATE;

typedef struct HalfObjectState {
    ToggleVector applied;
    STATE state;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int32_t counter;
#endif
} HalfObjectState;


typedef struct ObjectState {
    ToggleVector applied;
    STATE state;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int32_t counter;
#endif
    int32_t pad[PAD_CACHE(sizeof(HalfObjectState))];
} ObjectState;

typedef union pointer_t {
	struct StructData{
        int64_t seq : 48;
        int32_t index : 16;
	} struct_data;
	int64_t raw_data;
} pointer_t;

// Shared variables
volatile pointer_t sp CACHE_ALIGN;

// Try to place a_toggles and anounce to 
// the same cache line
volatile ToggleVector a_toggles CACHE_ALIGN;
// _TVEC_BIWORD_SIZE_ is a performance workaround for 
// array announce. Size N_THREADS is algorithmically enough.
volatile ArgVal announce[N_THREADS + _TVEC_BIWORD_SIZE_] CACHE_ALIGN;
volatile ObjectState pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;

// Each thread owns a private copy of the following variables
__thread BackoffStruct backoff;
__thread ToggleVector mask;
__thread ToggleVector toggle;
__thread ToggleVector my_bit;
__thread int local_index = 0;

void SHARED_OBJECT_INIT(int pid) {
    if (pid == 0) {
        sp.struct_data.index = LOCAL_POOL_SIZE * N_THREADS;
        sp.struct_data.seq = 0;
        TVEC_SET_ZERO((ToggleVector *)&a_toggles);

        // OBJECT'S INITIAL VALUE
        // ----------------------

        TVEC_SET_ZERO((ToggleVector *)&pool[LOCAL_POOL_SIZE * N_THREADS].applied);
#ifdef DEBUG
        pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0;
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


static inline void fetchAndMultiply(HalfObjectState *lsp_data, volatile Object arg, int pid) {
    int i;

#ifdef DEBUG
    lsp_data->counter++;
#endif
    for (i = 0; i < OBJECT_SIZE; i++)
        lsp_data->state.Critical[i].obj += 1;
    lsp_data->ret[pid] = lsp_data->state.Critical[1].obj;
}


static inline Object apply_op(void (*sfunc)(HalfObjectState *, volatile Object, int), Object arg, int pid) {
    ToggleVector diffs, l_toggles;
    pointer_t old_sp, new_sp;
    HalfObjectState *sp_data, *lsp_data;
    int i, j, prefix, mybank;

    announce[pid] = arg;                                           // announce the operation
    mybank = TVEC_GET_BANK_OF_BIT(pid);
    TVEC_REVERSE_BIT(&my_bit, pid);
    TVEC_NEGATIVE_BANK(&toggle, &toggle, mybank);
    lsp_data = (HalfObjectState *)&pool[pid * LOCAL_POOL_SIZE + local_index];
    TVEC_ATOMIC_ADD_BANK(&a_toggles, &toggle, mybank);             // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
    backoff_play(&backoff);

    for (j = 0; j < 2; j++) {
        old_sp = sp;                                               // read reference to struct ObjectState
        sp_data = (HalfObjectState *)&pool[old_sp.struct_data.index];  // read reference of struct ObjectState in a local variable lsp
        TVEC_ATOMIC_COPY_BANKS(&diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(&diffs, &diffs, &my_bit, mybank);           // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                               // if the operation has already been applied return
            break;
        *lsp_data = *sp_data;
        FullFence();
        l_toggles = a_toggles;                                     // This is an atomic read, since a_toogles is volatile
        if (old_sp.raw_data != sp.raw_data)
            continue;
        diffs = TVEC_XOR(lsp_data->applied, l_toggles);
#if N_THREADS > 2
        if (j == 0) 
            backoffCalculate(&backoff, TVEC_COUNT_BITS(diffs));
#endif
        sfunc(lsp_data, arg, pid);
        TVEC_REVERSE_BIT(&diffs, pid);
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            ReadPrefetch(&announce[prefix]);
            ReadPrefetch(&announce[prefix + 8]);
            ReadPrefetch(&announce[prefix + 16]);
            ReadPrefetch(&announce[prefix + 24]);
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                diffs.cell[i] ^= 1L << pos;
                sfunc(lsp_data, announce[proc_id], proc_id);
            }
        }
        lsp_data->applied = l_toggles;                                   // change applied to be equal to what was read in a_toggles
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;             // increase timestamp
        new_sp.struct_data.index = LOCAL_POOL_SIZE * pid + local_index;  // store in mod_dw.index the index in pool where lsp will be stored
        if (old_sp.raw_data==sp.raw_data && 
            CAS64(&sp, old_sp.raw_data, new_sp.raw_data)) {              // try to change sp to the value mod_dw
            local_index = (local_index + 1) % LOCAL_POOL_SIZE;           // if this happens successfully,use next item in pid's pool next time
            return lsp_data->ret[pid];
        }
    }
    LoadFence();
    old_sp.raw_data = sp.raw_data;                                       // after two unsuccessful efforts, read current value of sp
    return pool[old_sp.struct_data.index].ret[pid];                      // return the value found in the record stored there
}

pthread_barrier_t barr;
int64_t d1 CACHE_ALIGN, d2;

static inline void Execute(void* Arg) {
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
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        apply_op(fetchAndMultiply, (Object) (id + 1L), id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    stop_cpu_counters(id);
}

static inline void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
}

static inline pthread_t StartThread(int arg) {
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
    fprintf(stderr, "Object state long value: %lld\n", (long long int)pool[((pointer_t*)&sp)->struct_data.index].counter);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
