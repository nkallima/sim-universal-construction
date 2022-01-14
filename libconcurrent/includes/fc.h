/// @file fc.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the flat-combining synchronization technique (or shortly FC).
/// This code is an alternative implementation of flat-combining provided by the Synch framework and it has NO relation with the original code of flat-combining provided in
/// https://github.com/mit-carbon/Flat-Combining, since it is written from the scratch.
/// In many cases, this code performs much better than the original flat-combining implementation. An example of use of this API is provided in benchmarks/fcbench.c file.
/// Stack and queue implementations based on this implementation of flat-combining are provided (see fcstack.c and fcqueue.c).
///
/// For a more detailed description of flat-combining, see the original publication:
/// Danny Hendler, Itai Incze, Nir Shavit, and Moran Tzafrir. Flat combining and the synchronization-parallelism tradeoff. 
/// In Proceedings of the twenty-second annual ACM symposium on Parallelism in algorithms and architectures (SPAA 2010), pp. 355-364.
/// @copyright Copyright (c) 2021
#ifndef _FC_H_
#define _FC_H_

#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "system.h"

/// @brief HalfFCRequest should not be directly used by the user.
/// It is internally used for proper alignment of the FCRequest struct.
typedef struct HalfFCRequest {
    volatile struct FCRequest *next;
    volatile ArgVal val;
    volatile int age;
    volatile bool active;
    volatile bool pending;
} HalfFCRequest;

typedef struct FCRequest {
    /// @brief A pointer to the next announced request.
    volatile struct FCRequest *next;
    /// @brief This variable stores the argument of the request and the return value after the request is applied.
    volatile ArgVal val;
    /// @brief The age of the current announcement record.
    volatile int age;
    /// @brief If the request is active or not.
    volatile bool active;
    /// @brief If the request is pending or not.
    volatile bool pending;
    /// @brief Padding space.
    char pad[CACHE_LINE_SIZE - sizeof(HalfFCRequest)];
} FCRequest;

/// @brief FCStruct stores the state of an instance of the a FC object.
/// FCStruct should be initialized using the FCStructInit function.
typedef struct FCStruct {
    /// @brief A pointer to the head request.
    volatile struct FCRequest *head CACHE_ALIGN;
    /// @brief A central lock.
    volatile uint64_t lock CACHE_ALIGN;
    /// @brief The array of requests, one per thread.
    FCRequest *nodes;
    volatile uint64_t count CACHE_ALIGN;
    /// @brief This field counts the applied requests.
    volatile uint64_t counter;
    /// @brief The total number of executed combining rounds.
    volatile uint64_t rounds;
} FCStruct;

/// @brief FCThreadState stores each thread's local state for a single instance of FC.
/// For each instance of FC, a discrete instance of FCThreadState should be used.
typedef struct FCThreadState {
    FCRequest *node;
} FCThreadState;

/// @brief This function initializes an instance of the FC object.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the FCApplyOp function.
///
/// @param l A pointer to an instance of the FC object.
/// @param nthreads The number of threads that will use the FC object.
void FCStructInit(FCStruct *l, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the FC object.
///
/// @param l A pointer to an instance of the FC object.
/// @param st_thread A pointer to thread's local state of FC.
/// @param pid The pid of the calling thread.
void FCThreadStateInit(FCStruct *l, FCThreadState *st_thread, int pid);

/// @brief This function is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param l A pointer to an instance of the FC object.
/// @param st_thread A pointer to thread's local state for a specific instance of FC.
/// @param sfunc A serial function that the FC instance should execute, while applying requests announced by active threads.
/// @param state A pointer to the state of the simulated object.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling thread.
/// @return RetVal The return value of the applied request.
RetVal FCApplyOp(FCStruct *l, FCThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif