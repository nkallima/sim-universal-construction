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

#define POP                        -1


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


typedef union pointer_t {
	struct StructData{
        int64_t seq : 48;
        int32_t index : 16;
	} struct_data;
	int64_t raw_data;
} pointer_t;

// Shared variables
volatile pointer_t sp CACHE_ALIGN;
volatile ToggleVector a_toggles CACHE_ALIGN;
// _TVEC_BIWORD_SIZE_ is a performance workaround for 
// array announce. Size N_THREADS is algorithmically enough.
volatile ArgVal announce[N_THREADS + _TVEC_BIWORD_SIZE_] CACHE_ALIGN;
volatile ObjectState pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;


typedef struct SimStackThreadState {
    PoolStruct pool_node;
    BackoffStruct backoff;
    ToggleVector mask CACHE_ALIGN;
    ToggleVector toggle;
    ToggleVector my_bit;
    int local_index;
} SimStackThreadState;

void simStackThreadStateInit(SimStackThreadState *th_data, int pid) {
    th_data->local_index = 0;
    TVEC_SET_ZERO(&th_data->mask);
    TVEC_SET_ZERO(&th_data->my_bit);
    TVEC_SET_ZERO(&th_data->toggle);
    TVEC_REVERSE_BIT(&th_data->my_bit, pid);
    TVEC_SET_BIT(&th_data->mask, pid);
    th_data->toggle = TVEC_NEGATIVE(th_data->mask);

    init_backoff(&th_data->backoff, MIN_BAK, MAX_BAK, 1);
    init_pool(&th_data->pool_node, sizeof(Node));
}



void SHARED_OBJECT_INIT(int pid) {
    if (pid == 0) {
        sp.struct_data.index = LOCAL_POOL_SIZE * N_THREADS;
        sp.struct_data.seq = 0;
        TVEC_SET_ZERO((ToggleVector *)&a_toggles);

        // OBJECT'S INITIAL VALUE
        // ----------------------
        pool[LOCAL_POOL_SIZE * N_THREADS].head = null;

        TVEC_SET_ZERO((ToggleVector *)&pool[LOCAL_POOL_SIZE * N_THREADS].applied);
#ifdef DEBUG
        pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0;
#endif
        FullFence();
    }
}

inline void push(SimStackThreadState *th_data, HalfObjectState *st, ArgVal arg) {
#ifdef DEBUG
    st->counter += 1;
#endif
    Node *n;
    n = alloc_obj(&th_data->pool_node);
    n->value = (ArgVal)arg;
    n->next = st->head;
    st->head = n;
}

inline void pop(HalfObjectState *st, int pid) {
#ifdef DEBUG
    st->counter += 1;
#endif
    if(st->head != null) {
        st->ret[pid] = (RetVal)st->head->value;
        st->head = (Node *)st->head->next;
    }
    else st->ret[pid] = (RetVal)-1;
}

inline RetVal apply_op(SimStackThreadState *th_data, ArgVal arg, int pid) {
    ToggleVector diffs, l_toggles, pops;
    pointer_t new_sp, old_sp;
    HalfObjectState *lsp_data, *sp_data;
    int i, j, prefix, mybank, push_counter;
    ArgVal tmp_arg;

    announce[pid] = arg;                                                  // announce the operation
    mybank = TVEC_GET_BANK_OF_BIT(pid);
    TVEC_REVERSE_BIT(&th_data->my_bit, pid);
    TVEC_NEGATIVE_BANK(&th_data->toggle, &th_data->toggle, mybank);
    lsp_data = (HalfObjectState *)&pool[pid * LOCAL_POOL_SIZE + th_data->local_index];
    TVEC_ATOMIC_ADD_BANK(&a_toggles, &th_data->toggle, mybank);                    // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
    backoff_play(&th_data->backoff);

    for (j = 0; j < 2; j++) {
        old_sp = sp;		// read reference to struct ObjectState
        sp_data = (HalfObjectState *)&pool[old_sp.struct_data.index];    // read reference of struct ObjectState in a local variable lsp_data
        TVEC_ATOMIC_COPY_BANKS(&diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(&diffs, &diffs, &th_data->my_bit, mybank);           // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                                      // if the operation has already been applied return
            break;
        *lsp_data = *sp_data;
        FullFence();
        l_toggles = a_toggles;                                            // This is an atomic read, since a_toogles is volatile
        if (old_sp.raw_data != sp.raw_data)
            continue;
        diffs = TVEC_XOR(lsp_data->applied, l_toggles);
#if N_THREADS > 2
        if (j == 0) 
            backoffCalculate(&th_data->backoff, TVEC_COUNT_BITS(diffs));
#endif
        push_counter = 0;
        TVEC_SET_ZERO(&pops);
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
                tmp_arg = announce[proc_id];
                if (tmp_arg == POP) {
                    pops.cell[i] |= 1L << pos;
                } else {
                    push(th_data, lsp_data, tmp_arg);
                    push_counter++;
                }
            }
        }
		
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (pops.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(pops.cell[i]);
                proc_id = prefix + pos;
                pops.cell[i] ^= 1L << pos;
                pop(lsp_data, proc_id);
            }
        }

        lsp_data->applied = l_toggles;                                       // change applied to be equal to what was read in a_toggles
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;                 // increase timestamp
        new_sp.struct_data.index = LOCAL_POOL_SIZE * pid + th_data->local_index;      // store in mod_dw.index the index in pool where lsp_data will be stored
        if (old_sp.raw_data==sp.raw_data &&
            CAS64(&sp.raw_data, old_sp.raw_data, new_sp.raw_data)) {
            th_data->local_index = (th_data->local_index + 1) % LOCAL_POOL_SIZE;
            break;
        }
        else rollback(&th_data->pool_node, push_counter);
    }
    if (arg == POP)
        return (RetVal)-1;
    LoadFence();
    old_sp.raw_data = sp.raw_data;                                                 // after two unsuccessful efforts, read current value of sp
    return pool[old_sp.struct_data.index].ret[pid];         // return the value found in the record stored there
}

pthread_barrier_t barr CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;


inline void Execute(void* Arg) {
    SimStackThreadState th_data;
    long i;
    long rnum;
    volatile long j;
    int id = (long) Arg;

    simSRandom(id + 1);
    simStackThreadStateInit(&th_data, id);
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
        apply_op(&th_data, (ArgVal) id + 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        apply_op(&th_data, (ArgVal) POP, id);
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
    fprintf(stderr, "Object state debug counter: %lld\n", (long long int)pool[((pointer_t*)&sp)->struct_data.index].counter);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
