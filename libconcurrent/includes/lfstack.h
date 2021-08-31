/// @file lfstack.h
/// @brief This file exposes the API of the lock-free LF-Stack concurrent stack implementation.
/// An example of use of this API is provided in benchmarks/lfstackbench.c file.
///
/// For a more detailed description see the original publication:
/// R. Kent Treiber. "Systems programming: Coping with parallelism". International Business Machines Incorporated, Thomas J. Watson Research Center, 1986.
#ifndef _LFSTACK_H_
#define _LFSTACK_H_

#include <config.h>
#include <queue-stack.h>
#include <primitives.h>
#include <backoff.h>
#include <pool.h>

/// @brief LFStackStruct stores the state of an instance of the LF-Stack concurrent stack implementation.
/// LFStackStruct should be initialized using the LFStackInit function.
typedef struct LFStackStruct {
    /// @brief A pointer to the top element of the stack.
    volatile Node *top;
} LFStackStruct;

/// @brief LFStackThreadState stores each thread's local state for a single instance of LF-Stack.
/// For each instance of LF-Stack, a discrete instance of LFStackThreadState should be used.
typedef struct LFStackThreadState {
    /// @brief A pool object per thread is used for fast memory allocation.
    SynchPoolStruct pool;
    /// @brief A backoff object per thread is used for reducing the contention while accessing the queue.
    SynchBackoffStruct backoff;
} LFStackThreadState;

/// @brief This function initializes an instance of the LF-Stack concurrent stack implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any push or pop operation.
///
/// @param l A pointer to an instance of the LF-Stack concurrent stack implementation.
void LFStackInit(LFStackStruct *l);

/// @brief This function should be called once before the thread applies any operation to the LF-Stack concurrent stack implementation.
///
/// @param th_state A pointer to thread's local state of LF-Stack.
/// @param min_back The minimum value for backoff (in most cases 0 is a good start).
/// @param max_back The maximum value for backoff (usually this is much lower than 100).
void LFStackThreadStateInit(LFStackThreadState *th_state, int min_back, int max_back);

/// @brief This function adds (i.e. pushes) a new element to the top of the stack.
/// This element has a value equal with arg.
///
/// @param l A pointer to an instance of the LF-Stack concurrent stack implementation.
/// @param th_state A pointer to thread's local state of LF-Stack.
/// @param arg The push operation will insert a new element to the stack with value equal to arg.
void LFStackPush(LFStackStruct *l, LFStackThreadState *th_state, ArgVal arg);

/// @brief This function removes (i.e. pops) an element from the top of the stack and returns its value.
///
/// @param l A pointer to an instance of the LF-Stack concurrent stack implementation.
/// @param th_state A pointer to thread's local state of LF-Stack.
/// @return The value of the removed element.
RetVal LFStackPop(LFStackStruct *l, LFStackThreadState *th_state);

#endif