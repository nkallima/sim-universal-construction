#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <sim.h>
#include <barrier.h>
#include <bench_args.h>

SimStruct sim_struct CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
Barrier bar CACHE_ALIGN;
BenchArgs bench_args CACHE_ALIGN;
int MAX_BACK CACHE_ALIGN;


inline static RetVal fetchAndMultiply(HalfSimObjectState *lsp_data, ArgVal arg, int pid);

inline static RetVal fetchAndMultiply(HalfSimObjectState *lsp_data, ArgVal arg, int pid) {
#ifdef DEBUG
    lsp_data->counter += 1;
#endif
    lsp_data->state.obj += 1;

    return lsp_data->state.obj;
}

inline static void *Execute(void* Arg) {
    SimThreadState th_state;
    long i, rnum;
    long id = (long) Arg;
    volatile long j;

    SimThreadStateInit(&th_state, bench_args.nthreads, id);
    fastRandomSetSeed((unsigned long)id + 1);
    BarrierWait(&bar);
    if (id == 0)
        d1 = getTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        SimApplyOp(&sim_struct, &th_state, fetchAndMultiply, (Object) (id + 1), id);
        rnum = fastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    parseArguments(&bench_args, argc, argv);

    SimInit(&sim_struct, bench_args.nthreads, bench_args.backoff_high);
    BarrierInit(&bar, bench_args.nthreads);
    StartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    JoinThreadsN(bench_args.nthreads - 1);
    d2 = getTimeMillis();

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int) (d2 - d1), bench_args.runs * bench_args.nthreads/(1000.0*(d2 - d1)));
    printStats(bench_args.nthreads);

    
#ifdef DEBUG
    SimObjectState *l = (SimObjectState *)sim_struct.pool[((pointer_t*)&sim_struct.sp)->struct_data.index];
    fprintf(stderr, "DEBUG: Object state long value: %ld\n", l->state.obj);
    fprintf(stderr, "DEBUG: object counter: %d\n", l->counter);
    fprintf(stderr, "DEBUG: rounds: %d\n", l->rounds);
    fprintf(stderr, "DEBUG: Average helping: %f\n", (float)l->counter/l->rounds);
    fprintf(stderr, "\n");
#endif

    return 0;
}
