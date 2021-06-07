/// @file oscistack.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the OsciStack concurrent stack implementation.
/// An example of use of this API is provided in benchmarks/oscistackbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis. "Lock Oscillation: Boosting the Performance of Concurrent Data Structures",
/// In Proceedings of the 21st International Conference on Principles of Distributed Systems (Opodis), 2017.
/// @copyright Copyright (c) 2021
#ifndef _OSCISTACK_H_
#define _OSCISTACK_H_

#include <config.h>
#include <primitives.h>
#include <queue-stack.h>
#include <osci.h>
#include <pool.h>

/// @brief OsciStackStruct stores the state of an instance of the OsciStack concurrent stack implementation.
/// OsciStackStruct should be initialized using the OsciStackStructInit function.
typedef struct OsciStackStruct {
    /// @brief An Osci instance for servicing both push and pop operations.
    OsciStruct object_struct CACHE_ALIGN;
    /// @brief A pointer to the top element of the stack.
    volatile Node *top CACHE_ALIGN;
    /// @brief Pointer to an array of pools of nodes (a single pool per fiber). It is used for fast node allocation on push operations.
    PoolStruct *pool_node CACHE_ALIGN;
} OsciStackStruct;

/// @brief OsciStackThreadState stores each thread's local state for a single instance of OsciStack.
/// For each instance of OsciStack, a discrete instance of OsciStackThreadState should be used.
typedef struct OsciStackThreadState {
    /// @brief An OsciThreadState struct for the instance of Osci that serves both push and pop operations.
    OsciThreadState th_state;
} OsciStackThreadState;

/// @brief This function initializes an instance of the OsciStack concurrent stack implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any push or pop operation.
///
/// @param stack_object_struct A pointer to an instance of the OsciStack concurrent stack implementation.
/// @param nthreads The number of threads that will use the OsciStack concurrent stack implementation.
/// @param fibers_per_thread The number of fibers per Posix thread
void OsciStackInit(OsciStackStruct *stack_object_struct, uint32_t nthreads, uint32_t fibers_per_thread);

/// @brief This function should be called once before the thread applies any operation to the OsciStack concurrent stack implementation.
///
/// @param object_struct A pointer to an instance of the OsciStack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of OsciStack.
/// @param pid The pid of the calling thread.
void OsciStackThreadStateInit(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. pushes) a new element to the top of the stack.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the OsciStack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of OsciStack.
/// @param arg The push operation will insert a new element to the stack with value equal to arg.
/// @param pid The pid of the calling thread.
void OsciStackApplyPush(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. pops) an element from the top of the stack and returns its value.
///
/// @param object_struct A pointer to an instance of the OsciStack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of OsciStack.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal OsciStackApplyPop(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, int pid);

#endif
