#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>
#include <fcheap.h>
#include <barrier.h>
#include <bench_args.h>

FCHeapStruct *object_struct CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

inline static void *Execute(void* Arg) {
    FCHeapThreadState *th_state;
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    synchFastRandomSetSeed(id + 1);
    th_state = synchGetAlignedMemory(CACHE_LINE_SIZE, sizeof(FCHeapThreadState));
    FCHeapThreadStateInit(object_struct, th_state, (int)id);
    if (id == 0) {
        for (i = 0; i < SYNCH_HEAP_INITIAL_SIZE/2; i++)
             FCHeapInsert(object_struct, th_state, i, 0);
    }
    synchBarrierWait(&bar);
    if (id == 0)
        d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        // perform an insert operation
        FCHeapInsert(object_struct, th_state, bench_args.runs - i, id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ; 
        // perform a delete min operation
        SynchHeapElement h = FCHeapDeleteMin(object_struct, th_state, id);
        if (h==SYNCH_HEAP_EMPTY)
            fprintf(stderr, "DEBUG: Empty!\n");
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    synchParseArguments(&bench_args, argc, argv);
    object_struct = synchGetAlignedMemory(S_CACHE_LINE_SIZE, sizeof(FCHeapStruct));
    FCHeapInit(object_struct, FCHEAP_TYPE_MIN, bench_args.nthreads);
    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads - 1);
    d2 = synchGetTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), 2 * bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: Object state: %lld\n", object_struct->heap.counter - SYNCH_HEAP_INITIAL_SIZE/2);
    fprintf(stderr, "DEBUG: rounds: %ld\n", object_struct->heap.rounds);
    fprintf(stderr, "DEBUG: initial_items: %lld\n", SYNCH_HEAP_INITIAL_SIZE/2);
    fprintf(stderr, "DEBUG: remained_items: %ld\n", object_struct->state.items);
    fprintf(stderr, "DEBUG: last_level_used: %u\n", object_struct->state.last_used_level);
    fprintf(stderr, "DEBUG: last_used_level_pos: %u\n", object_struct->state.last_used_level_pos);
    fprintf(stderr, "DEBUG: Checking heap state: %s\n", ((serialHeapClearAndValidation(&object_struct->state) == true) ? "VALID" : "INVALID"));
#endif

    return 0;
}
