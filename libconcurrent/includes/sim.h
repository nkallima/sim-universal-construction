/// @file sim.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the P-Sim (or Sim) universal construction.
/// An example of use of this API is provided in benchmarks/simbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis. "A highly-efficient wait-free universal construction". 
/// Proceedings of the twenty-third annual ACM symposium on Parallelism in algorithms and architectures (SPAA), 2011.
/// @copyright Copyright (c) 2021
#ifndef _SIM_H_
#define _SIM_H_

#include <config.h>
#include <stdint.h>
#include <primitives.h>
#include <tvec.h>
#include <fam.h>

/// @brief This constant controls the size of pool of SimObjectState structs that each thread maintains.
/// This should be a small integer (i.e, 2 or 4) in order to avoid excess memory consumption.
#define _SIM_LOCAL_POOL_SIZE_ 4

#if _SIM_LOCAL_POOL_SIZE_ < 2
#    error SIM universal construction is improperly configured
#endif

/// @brief This should not used directely by user. 
/// It is used for padding the SimObjectState struct appropriately.
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

/// @brief This struct stores the data for a copy of the simulated object's state.
typedef struct SimObjectState {
    /// @brief A pointer to the array of return values.
    RetVal *ret;
    /// @brief The applied vector of toggles.
    ToggleVector applied;
    /// @brief The actual data of the simulated object's state.
    ObjectState state CACHE_ALIGN;
#ifdef DEBUG
    int counter;
    int rounds;
#endif
    uint64_t __flex[1];
    char pad[PAD_CACHE(sizeof(HalfSimObjectState))];
} SimObjectState;

/// @brief A macro for calculating the size of the SimObjectState struct for a specific amount of threads.
#define SimObjectStateSize(nthreads) (sizeof(SimObjectState) + _TVEC_VECTOR_SIZE(nthreads) + (nthreads) * sizeof(RetVal))

/// @brief pointer_t should not used directely by user. This struct is used by Sim for pointing to the 
/// most rescent and valid copy of the simulated object's state. It also contains a 40-bit sequence number
/// for avoiding the ABA problem.
typedef union pointer_t {
    struct StructData {
        uint64_t seq : 40;
        uint32_t index : 24;
    } struct_data;
    volatile uint64_t raw_data;
} pointer_t;

/// @brief SimThreadState stores each thread's local state for a single instance of Sim.
/// For each instance of Sim, a discrete instance of SimThreadState should be used.
typedef struct SimThreadState {
    ToggleVector mask;
    ToggleVector toggle;
    ToggleVector my_bit;
    ToggleVector diffs;
    ToggleVector l_toggles;
    /// @brief The next available free copy of object's state.
    int local_index;
    /// @brief Current backoff value.
    int backoff;
} SimThreadState;

/// @brief SimStruct stores the state of an instance of the a Sim combining object.
/// SimStruct should be initialized using the synchSimStructInit function.
typedef struct SimStruct {
    /// @brief Pointer to a SimObjectState structs that contains the most recent and valid copy of simulated object's state.
    volatile pointer_t sp;

    /// @brief Toggle bits.
    ToggleVector a_toggles CACHE_ALIGN;
    /// @brief An array of pools (one pool per thread) of SimObjectState structs.
    SimObjectState **volatile pool;
    /// @brief Pointer to an array, where threads announce the requests that want to perform to the object.
    ArgVal *volatile announce;

    /// @brief The number of threads that use this instance of Sim.
    uint32_t nthreads;
    /// @brief The maximum backoff value.
    int MAX_BACK;
} SimStruct;

/// @brief This function initializes an instance of the Sim universal construction.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the synchSimStructInit function.
///
/// @param sim_struct A pointer to an instance of the Sim universal construction.
/// @param nthreads The number of threads that will use the Sim universal construction.
/// @param max_backoff The maximum value for backoff (usually this is much lower than 100).
void synchSimStructInit(SimStruct *sim_struct, uint32_t nthreads, int max_backoff);

/// @brief This function should be called once before the thread applies any operation to the Sim universal construction.
///
/// @param th_state A pointer to thread's local state of Sim.
/// @param nthreads The number of threads that will use the Sim universal construction.
/// @param pid The pid of the calling thread.
void SimThreadStateInit(SimThreadState *th_state, uint32_t nthreads, int pid);

/// @brief This function is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param sim_struct A pointer to an instance of the Sim universal construction.
/// @param th_state A pointer to thread's local state for a specific instance of Sim.
/// @param sfunc A serial function that the Sim instance should execute, while applying requests announced by active threads.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling thread.
/// @return RetVal The return value of the applied request.
Object SimApplyOp(SimStruct *sim_struct, SimThreadState *th_state, RetVal (*sfunc)(void *, ArgVal, int), Object arg, int pid);

#endif
