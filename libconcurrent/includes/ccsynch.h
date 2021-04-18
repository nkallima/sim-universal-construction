/// @file ccsynch.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the CCSynch combining object.
/// An example of use of this API is provided in benchmarks/ccsynchbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021
#ifndef _CCSYNCH_H_
#define _CCSYNCH_H_

#include <config.h>
#include <primitives.h>

/// @brief HalfCCSynchNode should not be directly used by the user.
/// It is internally used for proper alignment of the CCSynchNode struct.
typedef struct HalfCCSynchNode {
    struct HalfCCSynchNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
} HalfCCSynchNode;

/// @brief CCSynchNode holds the state of an instance of the CCSynch object.
typedef struct CCSynchNode {
    /// @brief Pointer to the next node.
    struct CCSynchNode *next;
    /// @brief This variable stores the argument of the operation and the return value after the operation is applied.
    ArgVal arg_ret;
    /// @brief The pid of the thread.
    int32_t pid;
    /// @brief Whenever it is equal to false, the thread is the combiner; otherwise the thread waits.
    int32_t locked;
    /// @brief If true, the operation is applied and the thread returns its return value.
    int32_t completed;
    /// @brief Padding space.
    char align[PAD_CACHE(sizeof(HalfCCSynchNode))];
} CCSynchNode;

/// @brief CCSynchThreadState holds each thread's local state for a single instance of CCSynch.
/// For each instance of CCSynch, a discrete instance of CCSynchThreadState should be used.
typedef struct CCSynchThreadState {
    /// @brief pointer to the next request.
    CCSynchNode *next;
    /// @brief A toggled used by the CCSynch object.
    int toggle;
} CCSynchThreadState;

typedef struct CCSynchStruct {
    /// Tail points to the most recently announced request. Initially, it points to NULL.
    volatile CCSynchNode *Tail CACHE_ALIGN;
    /// Pointer to the pool of nodes used by threads in order to announce their requests.
    CCSynchNode *nodes CACHE_ALIGN;
    /// @brief The number of threads that will use the CCSynch combining object.
    uint32_t nthreads;
#ifdef DEBUG
    volatile uint64_t counter CACHE_ALIGN;
    volatile int rounds;
#endif
} CCSynchStruct;


/// @brief This function initializes an instance of the CCSynch combining object.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the CCSynchApplyOp function.
///
/// @param l A pointer to an instance of the CCSynch combining object.
/// @param nthreads The number of threads that will use the CCSynch combining object.
void CCSynchStructInit(CCSynchStruct *l, uint32_t nthreads);


/// @brief This function should be called once before the thread applies any operation to the CCSynch combining object.
///
/// @param l A pointer to an instance of the CCSynch combining object.
/// @param st_thread A pointer to thread's local state of CCSynch.
/// @param pid The pid of the calling thread.
void CCSynchThreadStateInit(CCSynchStruct *l, CCSynchThreadState *st_thread, int pid);


/// @brief This dunction is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param l A pointer to an instance of the CCSynch combining object.
/// @param st_thread A pointer to thread's local state for a specific instance of CCSynch.
/// @param sfunc A serial function that the CCSynch instance should execute, while applying requests of active threads.
/// @param state A pointer to the state of the simulated object.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling thread.
/// @return RetVal The return value of the applied request.
RetVal CCSynchApplyOp(CCSynchStruct *l, CCSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif
