/// @file oyama.h
/// @brief This file exposes the API of the CC-Synch combining object.
/// An example of use of this API is provided in benchmarks/oyamabench.c file.
///
/// For a more detailed description see the original publication:
/// Yoshihiro Oyama, Kenjiro Taura, and Akinori Yonezawa. "Executing parallel programs with synchronization bottlenecks efficiently". 
/// Proceedings of the International Workshop on Parallel and Distributed Computing for Symbolic and Irregular Applications. Vol. 16. 1999.
#ifndef _OYAMA_H_
#define _OYAMA_H_

#include <config.h>
#include <primitives.h>

/// @brief HalfOyamaAnnounceNode should not be directly used by the user.
/// It is internally used for proper alignment of the OyamaAnnounceNode struct.
typedef struct HalfOyamaAnnounceNode {
    volatile struct OyamaAnnounceNode *next;
    volatile ArgVal arg_ret;
    int32_t pid;
    volatile bool completed;
} HalfOyamaAnnounceNode;

/// @brief OyamaAnnounceNode stores the data of an announced request.
typedef struct OyamaAnnounceNode {
    /// @brief Pointer to the next request that has been announced.
    volatile struct OyamaAnnounceNode *next;
    /// @brief This variable stores the argument of the request and the return value after the request is applied.
    volatile ArgVal arg_ret;
    /// @brief The pid of the thread that announced this request.
    int32_t pid;
    /// @brief If true, the request is applied and the thread returns its return value.
    volatile bool completed;
    char align[PAD_CACHE(sizeof(HalfOyamaAnnounceNode))];
} OyamaAnnounceNode;

/// @brief OyamaStruct stores the state of an instance of the Oyama combining object.
/// OyamaStruct should be initialized using the OyamaInit function.
typedef struct OyamaStruct {
    /// @brief A global lock that protects accesses on the combining object.
    volatile int32_t lock CACHE_ALIGN;
    /// @brief Tail points to the most recently announced request. Initially, it points to NULL.
    volatile OyamaAnnounceNode *tail CACHE_ALIGN;
    /// @brief The number of threads that will use the Oyama combining object.
    uint32_t nthreads CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} OyamaStruct;

/// @brief OyamaThreadState stores each thread's local state for a single instance of the Oyama combining object.
/// For each instance of the Oyama combining object, a discrete instance of OyamaThreadState should be used.
typedef struct OyamaThreadState {
    /// @brief A struct for announcing requests.
    OyamaAnnounceNode my_node;
} OyamaThreadState;

/// @brief This function initializes an instance of the Oyama combining object.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the OyamaApplyOp function.
///
/// @param l A pointer to an instance of the Oyama combining object.
/// @param nthreads The number of threads that will use the Oyama combining object.
void OyamaInit(OyamaStruct *l, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the Oyama combining object.
///
/// @param th_state A pointer to thread's local state of the Oyama combining object.
void OyamaThreadStateInit(OyamaThreadState *th_state);

/// @brief This function is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param l A pointer to an instance of the Oyama combining object.
/// @param th_state A pointer to thread's local state for a specific instance of the Oyama combining object.
/// @param sfunc A serial function that the Oyama combining object instance should execute, while applying requests announced by active threads.
/// @param state A pointer to the state of the simulated object.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling thread.
/// @return RetVal The return value of the applied request.
RetVal OyamaApplyOp(volatile OyamaStruct *l, OyamaThreadState *th_state, RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid);
#endif
