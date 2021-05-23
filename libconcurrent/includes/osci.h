/// @file osci.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the Osci combining object.
/// An example of use of this API is provided in benchmarks/oscibench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis. "Lock Oscillation: Boosting the Performance of Concurrent Data Structures.
/// In proceedings of the 21st International Conference on Principles of Distributed Systems (OPODIS 2017), 2018.
/// @copyright Copyright (c) 2021
#ifndef __OSCI_H_
#define __OSCI_H_

#include <config.h>
#include <primitives.h>
#include <types.h>

enum { _OSCI_DOOR_INIT, _OSCI_DOOR_OPENED, _OSCI_DOOR_LOCKED };

/// @brief OsciFiberRec stores the data of an announced request by a fiber.
typedef struct OsciFiberRec {
    /// @brief This variable stores the argument of the request and the return value after the request is applied.
    volatile ArgVal arg_ret;
    /// @brief The pid of the fiber that announced this request.
    volatile int32_t pid;
    /// @brief Whenever it is equal to false, the fiber is the combiner; otherwise the fiber waits until a combiner apply its request.
    volatile int16_t locked;
    /// @brief If true, the request is applied and the fiber returns its return value.
    volatile int16_t completed;
} OsciFiberRec;

/// @brief OsciNode is used as a combining point for the fibers of the current posix thread.
typedef struct OsciNode {
    /// @brief next point to the next request announced by some fiber of the current posix thread.
    volatile struct OsciNode *next;
    /// @brief toggle bit for the current combining point. 
    volatile int32_t toggle;
    /// @brief door may be equal with any of the following: _OSCI_DOOR_INIT or _OSCI_DOOR_OPENED or _OSCI_DOOR_LOCKED.
    volatile int32_t door;
    /// @brief points to an announced request.
    volatile OsciFiberRec *rec;
} OsciNode;

/// @brief OsciThreadState stores each posix thread's local state for a single instance of Osci.
/// For each instance of Osci, a different instance of OsciThreadState should be used.
typedef struct OsciThreadState {
    /// @brief An array of two empty request structs that will be used for announcing future requests.
    volatile OsciNode next_node[2];
    /// @brief A toggle-bit used by the Osci object.
    int toggle;
} OsciThreadState;

/// @brief OsciStruct stores the state of an instance of the a Osci combining object.
/// OsciStruct should be initialized using the OSciInit function.
typedef struct OsciStruct {
    /// @brief Tail points to the most recently announced request. Initially, it points to NULL.
    volatile OsciNode *Tail CACHE_ALIGN;
    /// @brief The number of fibers (or threads) that will use the Osci combining object.
    uint32_t nthreads CACHE_ALIGN;
    /// @brief The number of fibers per posix thread that the current instance of Osci supports (this is set by using the OsciInit functionality).
    uint32_t fibers_per_thread;
    /// @brief The number of posix threads that are accessing the current instance of Osci. Each posix thread contains fibers_per_thread fibers.
    uint32_t groups_of_fibers;
    /// @brief Combining points for the current instance of Osci.
    ptr_aligned_t *current_node;
#ifdef DEBUG
    volatile uint64_t counter;
    volatile int rounds CACHE_ALIGN;
#endif
} OsciStruct;

/// @brief This function initializes an instance of the Osci combining object.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the OsciApplyOp function.
///
/// @param l A pointer to an instance of the Osci combining object.
/// @param nthreads The number of threads that will use the Osci combining object.
/// @param fibers_per_thread The number of fibers per posix thread that the current instance of Osci supports (this is set by using the OsciInit functionality).
void OsciInit(OsciStruct *l, uint32_t nthreads, uint32_t fibers_per_thread);

/// @brief This function should be called once before the thread applies any operation to the Osci combining object.
///
/// @param l A pointer to an instance of the Osci combining object.
/// @param st_thread A pointer to fiber's (or thread's) local state of Osci.
/// @param pid The pid of the calling thread.
void OsciThreadStateInit(OsciThreadState *st_thread, OsciStruct *l, int pid);

/// @brief This function is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param l A pointer to an instance of the Osci combining object.
/// @param st_thread A pointer to thread's local state for a specific instance of OSci.
/// @param sfunc A serial function that the Osci instance should execute, while applying requests announced by active fibers (or threads).
/// @param state A pointer to the state of the simulated object.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling fiber (or thread).
/// @return RetVal The return value of the applied request.
RetVal OsciApplyOp(OsciStruct *l, OsciThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif
