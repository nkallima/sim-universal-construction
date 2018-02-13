#ifndef _SIM_H_
#define _SIM_H_

#include <stdint.h>
#include <config.h>
#include <primitives.h>
#include <tvec.h>
#include <fastrand.h>
#include <threadtools.h>

#define _SIM_LOCAL_POOL_SIZE_            4


typedef struct _SIM_STATE{
    volatile Object obj CACHE_ALIGN;
} _SIM_STATE;

typedef struct HalfSimObjectState {
    ToggleVector applied;
    _SIM_STATE state;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int counter;
    int rounds CACHE_ALIGN;
#endif
} HalfSimObjectState;

typedef struct SimObjectState {
    ToggleVector applied;
    _SIM_STATE state;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int counter;
    int rounds CACHE_ALIGN;
#endif
    int32_t pad[PAD_CACHE(sizeof(HalfSimObjectState))];
} SimObjectState;

typedef union pointer_t {
    struct StructData{
        int64_t seq : 32;
        int32_t index : 32;
    } struct_data;
    int64_t raw_data;
} pointer_t;

typedef struct SimThreadState {
    ToggleVector mask;
    ToggleVector toggle;
    ToggleVector my_bit;
    int local_index;
    int backoff;
} SimThreadState;

typedef struct SimStruct {
    int MAX_BACK;
    // Shared variables
    volatile pointer_t sp CACHE_ALIGN;
    // Try to place a_toggles and announce to 
    // the same cache line
    volatile ToggleVector a_toggles CACHE_ALIGN;
    // _TVEC_BIWORD_SIZE_ is a performance workaround for 
    // array announce. Size N_THREADS is algorithmically enough.
    volatile ArgVal announce[N_THREADS + _TVEC_BIWORD_SIZE_] CACHE_ALIGN;
    volatile SimObjectState pool[_SIM_LOCAL_POOL_SIZE_ * N_THREADS + 1] CACHE_ALIGN;
} SimStruct;

void SimInit(SimStruct *sim_struct, int max_backoff);
void SimThreadStateInit(SimThreadState *th_state, int pid);
Object SimApplyOp(SimStruct *sim_struct, SimThreadState *th_state, RetVal (*sfunc)(HalfSimObjectState *, ArgVal, int), Object arg, int pid);

#endif