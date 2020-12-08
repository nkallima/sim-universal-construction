#ifndef _SIM_H_
#define _SIM_H_

#include <stdint.h>
#include <config.h>
#include <primitives.h>
#include <tvec.h>
#include <fastrand.h>
#include <threadtools.h>
#include <fam.h>

#define _SIM_LOCAL_POOL_SIZE_ 4

#if _SIM_LOCAL_POOL_SIZE_ < 2
#    error SIM universal construction is mproperily configured
#endif

typedef struct HalfSimObjectState {
    RetVal *ret;
    ToggleVector applied;
    ObjectState state CACHE_ALIGN;
#ifdef DEBUG
    int counter;
    int rounds;
#endif
    uint64_t __flex[1];
} HalfSimObjectState;

typedef struct SimObjectState {
    RetVal *ret;
    ToggleVector applied;
    ObjectState state CACHE_ALIGN;
#ifdef DEBUG
    int counter;
    int rounds;
#endif
    uint64_t __flex[1];
    char pad[PAD_CACHE(sizeof(HalfSimObjectState))];
} SimObjectState;

#define SimObjectStateSize(nthreads) (sizeof(SimObjectState) + _TVEC_VECTOR_SIZE(nthreads) + (nthreads) * sizeof(RetVal))

typedef union pointer_t {
    struct StructData {
        int64_t seq : 32;
        int32_t index : 32;
    } struct_data;
    int64_t raw_data;
} pointer_t;

typedef struct SimThreadState {
    ToggleVector mask;
    ToggleVector toggle;
    ToggleVector my_bit;
    ToggleVector diffs;
    ToggleVector l_toggles;
    int local_index;
    int backoff;
} SimThreadState;

typedef struct SimStruct {
    volatile pointer_t sp;

    // Pointers toi shared data
    ToggleVector a_toggles CACHE_ALIGN;
    SimObjectState **volatile pool;
    ArgVal *volatile announce;

    // Some constants
    uint32_t nthreads;
    int MAX_BACK;
} SimStruct;

void SimInit(SimStruct *sim_struct, uint32_t nthreads, int max_backoff);
void SimThreadStateInit(SimThreadState *th_state, uint32_t nthreads, int pid);
Object SimApplyOp(SimStruct *sim_struct, SimThreadState *th_state, RetVal (*sfunc)(void *, ArgVal, int), Object arg, int pid);

#endif
