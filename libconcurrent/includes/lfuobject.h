/// @file lfuobject.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of a simple lock-free universal construction (for small objects) that implements the atomic operation provided by the fam.h file.
/// This lock-free implementation supports backoff for reducing the contention.
/// An example of use of this API is provided in benchmarks/lfuobjectbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021
#ifndef _LFUOBJECT_H_
#define _LFUOBJECT_H_

#include <config.h>
#include <primitives.h>

#include <backoff.h>
#include <fam.h>

/// @brief LFUObjectStruct stores the state of an instance of the a lock-free universal construction.
/// LFUObjectStruct should be initialized using the LFUObjectStructInit function.
typedef union LFUObjectStruct {
    /// @brief This field stores the state of the concurrent object. This variable is of type ObjectState, which its size is usually 64-bit (defined in config.h).
    /// Note that the machine should be provide atomic Read/Write and Compare&Swap (or LL/SC) acceses to this variable type.
    volatile ObjectState state;
    /// @brief padding space.
    char pad[CACHE_LINE_SIZE];
} LFUObjectStruct;

/// @brief LFUObjectThreadState stores each thread's local state for a single instance of the universal construction.
/// For each instance of the lock-free universal construction, a discrete instance of LFUObjectThreadState should be used.
typedef struct LFUObjectThreadState {
    /// @brief A backoff object per thread is used for reducing the contention while accessing the object.
    BackoffStruct backoff;
} LFUObjectThreadState;

/// @brief This function initializes an instance of the lock-free universal construction.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the LFUObjectApplyOp function.
///
/// @param l A pointer to an instance of the lock-free universal construction.
/// @param value The initial value that the simulated object should have.
void LFUObjectInit(LFUObjectStruct *l, ArgVal value);

/// @brief This function should be called once before the thread applies any operation to the lock-free universal construction.
///
/// @param th_state A pointer to thread's local state of lock-free.
/// @param min_back The minimum value for backoff (in most cases 0 is a good start).
/// @param max_back The maximum value for backoff (usually this is much lower than 100).
void LFUObjectThreadStateInit(LFUObjectThreadState *th_state, int min_back, int max_back);

/// @brief This function is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param l A pointer to an instance of the lock-free universal construction.
/// @param th_state A pointer to thread's local state for a specific instance of lock-free.
/// @param sfunc A serial function that the lock-free instance should execute, while applying requests announced by active threads.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling thread.
/// @return RetVal The return value of the applied request.
RetVal LFUObjectApplyOp(LFUObjectStruct *l, LFUObjectThreadState *th_state, RetVal (*sfunc)(void *, ArgVal, int), ArgVal arg, int pid);

#endif
