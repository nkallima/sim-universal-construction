#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <clh.h>
#include <threadtools.h>
#include <barrier.h>
#include <bench_args.h>
#include <fam.h>

CLHLockStruct *object_lock CACHE_ALIGN;
ObjectState object CACHE_ALIGN;
#ifdef DEBUG
volatile uint64_t debug_state = 0;
#endif
int64_t d1 CACHE_ALIGN, d2;
SynchBarrier bar CACHE_ALIGN;
SynchBenchArgs bench_args CACHE_ALIGN;

inline void apply_op(RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    CLHLock(object_lock, pid);
    sfunc(state, arg, pid);
#ifdef DEBUG
    debug_state += 1;
#endif
    CLHUnlock(object_lock, pid);
}

inline static void *Execute(void *Arg) {
    long i, rnum;
    volatile long j;
    long id = (long)Arg;

    synchFastRandomSetSeed(id + 1);
    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < bench_args.runs; i++) {
        apply_op(fetchAndMultiply, &object, (ArgVal)i, (int)id);
        rnum = synchFastRandomRange(1, bench_args.max_work);
        for (j = 0; j < rnum; j++)
            ;
    }
    synchBarrierWait(&bar);
    if (id == 0) d2 = synchGetTimeMillis();

    return NULL;
}

int main(int argc, char *argv[]) {
    synchParseArguments(&bench_args, argc, argv);
    object.state_f = 1.0;
    object_lock = CLHLockInit(bench_args.nthreads);

    synchBarrierSet(&bar, bench_args.nthreads);
    synchStartThreadsN(bench_args.nthreads, Execute, bench_args.fibers_per_thread);
    synchJoinThreadsN(bench_args.nthreads);

    printf("time: %d (ms)\tthroughput: %.2f (millions ops/sec)\t", (int)(d2 - d1), bench_args.runs * bench_args.nthreads / (1000.0 * (d2 - d1)));
    synchPrintStats(bench_args.nthreads, bench_args.total_runs);

#ifdef DEBUG
    fprintf(stderr, "DEBUG: Object state: %lu\n", debug_state);
    fprintf(stderr, "DEBUG: Object float state: %f\n", object.state_f);
#endif

    return 0;
}
