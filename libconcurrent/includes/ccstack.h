/// @file ccstack.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the CC-Stack concurrent stack implementation.
/// An example of use of this API is provided in benchmarks/ccstackbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021
#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include <config.h>
#include <queue-stack.h>
#include <ccsynch.h>

/// @brief CCStackStruct stores the state of an instance of the CC-Stack concurrent stack implementation.
/// CCStackStruct should be initialized using the CCStackStructInit function.
typedef struct CCStackStruct {
    /// @brief A CC-Synch instance for servicing both push and pop operations.
    CCSynchStruct object_struct CACHE_ALIGN;
    /// @brief A pointer to the top element of the stack.
    volatile Node *volatile top CACHE_ALIGN;
} CCStackStruct;

/// @brief CCStackThreadState stores each thread's local state for a single instance of CC-Stack.
/// For each instance of CC-Stack, a discrete instance of CCStackThreadState should be used.
typedef struct CCStackThreadState {
    /// @brief A CCSynchThreadState struct for the instance of CC-Synch that serves both push and pop operations.
    CCSynchThreadState th_state;
} CCStackThreadState;

/// @brief This function initializes an instance of the CC-Stack concurrent stack implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any push or pop operation.
///
/// @param stack_object_struct A pointer to an instance of the CC-Stack concurrent stack implementation.
/// @param nthreads The number of threads that will use the CC-Stack concurrent stack implementation.
void CCStackInit(CCStackStruct *stack_object_struct, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the CC-Stack concurrent stack implementation.
///
/// @param object_struct A pointer to an instance of the CC-Stack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of CC-Stack.
/// @param pid The pid of the calling thread.
void CCStackThreadStateInit(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid);

/// @brief This function adds (i.e. pushes) a new element to the top of the stack.
/// This element has a value equal with arg.
///
/// @param object_struct A pointer to an instance of the CC-Stack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of CC-Stack.
/// @param arg The push operation will insert a new element to the stack with value equal to arg.
/// @param pid The pid of the calling thread.
void CCStackPush(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, ArgVal arg, int pid);

/// @brief This function removes (i.e. pops) an element from the top of the stack and returns its value.
///
/// @param object_struct A pointer to an instance of the CC-Stack concurrent stack implementation.
/// @param lobject_struct A pointer to thread's local state of CC-Stack.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal CCStackPop(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid);

#endif
