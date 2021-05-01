/// @file dsmsynch.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the DSMSynch combining object.
/// An example of use of this API is provided in benchmarks/dsmsynchbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021
#ifndef _DSMSYNCH_H_
#define _DSMSYNCH_H_

#include <config.h>
#include <primitives.h>

/// @brief HalfDSMSynchNode should not be directly used by the user.
/// It is internally used for proper alignment of the DSMSynchNode struct.
typedef struct HalfDSMSynchNode {
    struct DSMSynchNode *next;
    ArgVal arg;
    RetVal ret;
    uint32_t pid;
    volatile uint32_t locked;
    volatile uint32_t completed;
} HalfDSMSynchNode;

/// @brief DSMSynchNode stores the data of an announced request.
typedef struct DSMSynchNode {
    /// @brief Pointer to the next request that has been announced.
    struct DSMSynchNode *next;
    /// @brief This variable stores the argument of the request and the return value after the request is applied.
    ArgVal arg_ret;
    /// @brief The pid of the thread that announced this request.
    uint32_t pid;
    /// @brief Whenever it is equal to false, the thread is the combiner; otherwise the thread waits until a combiner apply its request.
    volatile uint32_t locked;
    /// @brief If true, the request is applied and the thread returns its return value.
    volatile uint32_t completed;
    /// @brief Padding space.
    char align[PAD_CACHE(sizeof(HalfDSMSynchNode))];
} DSMSynchNode;

/// @brief DSMSynchThreadState stores each thread's local state for a single instance of DSMSynch.
/// For each instance of DSMSynch, a discrete instance of DSMSynchThreadState should be used.
typedef struct DSMSynchThreadState {
    /// @brief A pool of nodes used for announcing requests.
    DSMSynchNode *MyNodes[2];
    /// @brief A toggle-bit used by the DSMSynch object.
    int toggle;
} DSMSynchThreadState;

/// @brief DSMSynchStruct stores the state of an instance of the a DSMSynch combining object.
/// DSMSynchStruct should be initialized using the DSMSynchStructInit function.
typedef struct DSMSynchStruct {
    /// @brief Tail points to the most recently announced request. Initially, it points to NULL.
    volatile DSMSynchNode *Tail CACHE_ALIGN;
    /// @brief Pointer to the pool of nodes used by threads in order to announce their requests.
    DSMSynchNode *nodes CACHE_ALIGN;
    /// @brief The number of threads that will use the DSMSynch combining object.
    uint32_t nthreads;
#ifdef DEBUG
    volatile uint64_t counter CACHE_ALIGN;
    volatile int rounds;
#endif
} DSMSynchStruct;

/// @brief This function initializes an instance of the DSMSynch combining object.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the DSMSynchApplyOp function.
///
/// @param l A pointer to an instance of the DSMSynch combining object.
/// @param nthreads The number of threads that will use the DSMSynch combining object.
void DSMSynchStructInit(DSMSynchStruct *l, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the DSMSynch combining object.
///
/// @param l A pointer to an instance of the DSMSynch combining object.
/// @param st_thread A pointer to thread's local state of DSMSynch.
/// @param pid The pid of the calling thread.
void DSMSynchThreadStateInit(DSMSynchStruct *l, DSMSynchThreadState *st_thread, int pid);

/// @brief This function is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param l A pointer to an instance of the DSMSynch combining object.
/// @param st_thread A pointer to thread's local state for a specific instance of DSMSynch.
/// @param sfunc A serial function that the DSMSynch instance should execute, while applying requests announced by active threads.
/// @param state A pointer to the state of the simulated object.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling thread.
/// @return RetVal The return value of the applied request.
RetVal DSMSynchApplyOp(DSMSynchStruct *l, DSMSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif
