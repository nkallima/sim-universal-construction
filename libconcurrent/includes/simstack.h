/// @file simstack.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the SimStack concurrent stack implementation.
/// An example of use of this API is provided in benchmarks/simstackbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis. "A highly-efficient wait-free universal construction". 
/// Proceedings of the twenty-third annual ACM symposium on Parallelism in algorithms and architectures (SPAA), 2011.
/// @copyright Copyright (c) 2021
#ifndef _SIMSTACK_H_
#define _SIMSTACK_H_

#include <sim.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

/// @brief This struct is  used for padding the SimStackState struct appropriately.
typedef struct HalfSimStackState {
    ToggleVector applied;
    Object *ret;
    Node *head;
#ifdef DEBUG
    int counter;
#endif
    uint64_t __flex[1];
} HalfSimStackState;

/// @brief This struct stores the data about the state of a SimStack instance.
typedef struct SimStackState {
    /// @brief The applied vector of toggles.
    ToggleVector applied;
    /// @brief A pointer to the array of return values.
    Object *ret;
    /// @brief A pointer to the head node of the stack.
    Node *head;
#ifdef DEBUG
    int counter;
#endif
    uint64_t __flex[1];
    /// @brief Padding space.
    char pad[PAD_CACHE(sizeof(HalfSimStackState))];
} SimStackState;

/// @brief A macro for calculating the size of the SimStackState struct for a specific amount of threads.
#define SimStackStateSize(nthreads) (sizeof(SimStackState) + _TVEC_VECTOR_SIZE(nthreads) + nthreads * sizeof(Object))

/// @brief SimStackThreadState stores each thread's local state for a single instance of SimStack.
/// For each instance of SimStack, a discrete instance of SimStackThreadState should be used.
typedef struct SimStackThreadState {
    /// @brief A pool of Node structs used for fast allocation during push operations.
    PoolStruct pool;
    ToggleVector mask;
    ToggleVector toggle;
    ToggleVector my_bit;
    ToggleVector diffs;
    ToggleVector l_toggles;
    ToggleVector pops;
    /// @brief The next available free copy of SimStackState that could be used while applying push and/or pop operations.
    int local_index;
    /// @brief The maximum backoff value.
    int backoff;
} SimStackThreadState;

/// @brief SimStackStruct stores the state of an instance of the SimStack.
/// SimStackStruct should be initialized using the SimStackStructInit function.
typedef struct SimStackStruct {
    /// @brief Pointer to an array, where threads announce only the requests that want to perform to the object.
    ArgVal *announce;
    /// @brief An array of pools (one pool per thread) of SimStackState structs.
    SimStackState **pool;
    /// @brief The number of threads that use this instance of SimStack.
    uint32_t nthreads;
    /// @brief The maximum backoff value.
    int MAX_BACK;
    /// @brief A pointer to the head node of the stack.
    volatile Node *head CACHE_ALIGN;
    /// @brief Pointer to a SimStackState struct that contains the most recent and valid copy of this SimStack instance.
    volatile pointer_t sp CACHE_ALIGN;
    /// @brief A vector of toggle bits used to detect which announced operations are applied or not.
    volatile ToggleVector a_toggles CACHE_ALIGN;
} SimStackStruct;


/// @brief This function initializes an instance of the SimStack concurrent stack implementation.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any push or pop operation.
///
/// @param stack A pointer to an instance of the SimStack concurrent stack implementation.
/// @param nthreads The number of threads that will use the SimStack concurrent stack implementation.
/// @param max_backoff The maximum value for backoff (usually this is much lower than 100).
void SimStackStructInit(SimStackStruct *stack, uint32_t nthreads, int max_backoff);

/// @brief This function should be called once before the thread applies any operation to the SimStack concurrent stack implementation.
///
/// @param th_state A pointer to thread's local state of SimStack.
/// @param nthreads The number of threads that will use the SimStack concurrent stack implementation.
/// @param pid The pid of the calling thread.
void SimStackThreadStateInit(SimStackThreadState *th_state, uint32_t nthreads, int pid);

/// @brief This function adds (i.e. pushes) a new element to the top of the stack.
/// This element has a value equal with arg.
///
/// @param stack A pointer to an instance of the SimStack concurrent stack implementation.
/// @param th_state A pointer to thread's local state of SimStack.
/// @param arg The push operation will insert a new element to the stack with value equal to arg.
/// @param pid The pid of the calling thread.
void SimStackPush(SimStackStruct *stack, SimStackThreadState *th_state, ArgVal arg, int pid);

/// @brief This function removes (i.e. pops) an element from the top of the stack and returns its value.
///
/// @param stack A pointer to an instance of the SimStack concurrent stack implementation.
/// @param th_state A pointer to thread's local state of SimStack.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal SimStackPop(SimStackStruct *stack, SimStackThreadState *th_state, int pid);

#endif
