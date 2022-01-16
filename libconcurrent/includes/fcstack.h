/// @file fcstack.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of a concurrent stack implementation using the flat-combining synchronization technique (or shortly FC).
/// The provided code of flat-combining is an alternative implementation of the Synch framework and it has NO relation with the original code of flat-combining provided in
/// https://github.com/mit-carbon/Flat-Combining, since it is written from the scratch.
/// In many cases, this code performs much better than the original flat-combining implementation. An example of use of this API is provided in benchmarks/fcstackbench.c file.
///
/// For a more detailed description of flat-combining, see the original publication:
/// Danny Hendler, Itai Incze, Nir Shavit, and Moran Tzafrir. Flat combining and the synchronization-parallelism tradeoff. 
/// In Proceedings of the twenty-second annual ACM symposium on Parallelism in algorithms and architectures (SPAA 2010), pp. 355-364.
/// @copyright Copyright (c) 2021
#ifndef _FCSTACK_H_
#define _FCSTACK_H_

#include <fc.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

/// @brief FCStackStruct stores the state of an instance of the FC-Stack concurrent stack implementation.
/// FCStackStruct should be initialized using the FCStackInit function.
typedef struct FCStackStruct {
    /// @brief A flat-combining instance for servicing both push and pop operations.
    FCStruct object_struct CACHE_ALIGN;
    /// @brief A pointer to the top element of the stack.
    volatile Node * volatile top CACHE_ALIGN;
} FCStackStruct;

/// @brief FCThreadState stores each thread's local state for a single instance of FC-Stack.
/// For each instance of FC-Stack, a discrete instance of FCThreadState should be used.
typedef struct FCStackThreadState {
    /// @brief FCThreadState struct for the instance of flat-combining that serves both push and pop operations.
    FCThreadState th_state;
} FCStackThreadState;

/// @brief This function initializes an instance of the FC-Stack concurrent stack implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any push or pop operation.
///
/// @param stack_object_struct A pointer to an instance of the FC-Stack concurrent stack implementation.
/// @param nthreads The number of threads that will use the FC-Stack concurrent stack implementation.
void FCStackInit(FCStackStruct *stack_object_struct, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the FC-Stack concurrent stack implementation.
///
/// @param object_struct A pointer to an instance of the FC-Stack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of FC-Stack.
/// @param pid The pid of the calling thread.
void FCStackThreadStateInit(FCStackStruct *object_struct, FCStackThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. pushes) a new element to the top of the stack.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the FC-Stack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of FC-Stack.
/// @param arg The push operation will insert a new element to the stack with value equal to arg.
/// @param pid The pid of the calling thread.
void FCStackPush(FCStackStruct *object_struct, FCStackThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. pops) an element from the top of the stack and returns its value.
///
/// @param object_struct A pointer to an instance of the FC-Stack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of FC-Stack.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal FCStackPop(FCStackStruct *object_struct, FCStackThreadState *lobject_struct, int pid);

#endif
