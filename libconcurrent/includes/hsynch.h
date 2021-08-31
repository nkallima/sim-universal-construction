/// @file hsynch.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file exposes the API of the HSynch combining object.
/// An example of use of this API is provided in benchmarks/hsynchbench.c file.
///
/// For a more detailed description see the original publication:
/// Panagiota Fatourou, and Nikolaos D. Kallimanis."Revisiting the combining synchronization technique".
/// ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.
/// @copyright Copyright (c) 2021
#ifndef _HSYNCH_H_
#define _HSYNCH_H_

#include <config.h>
#include <primitives.h>
#include <clh.h>

/// @brief Whenever numa_regions is equal to HSYNCH_DEFAULT_NUMA_POLICY, the user uses the default number of NUMA nodes,
/// which is equal to the number of NUMA nodes that the machine provides. The information about machine's NUMA 
/// characteristics is provided by the functionality of numa.h lib. In case that numa_regions is different than
/// HSYNCH_DEFAULT_NUMA_POLICY, the user overides system's default number of NUMA nodes. For example, if  numa_regions = 2
/// and the machine is equipped with 4 NUMA nodes,then the H-Synch will ignore this and will create a fictitious topology of 
/// 2 NUMA nodes. This is very usefull in cases of machines that provide many NUMA nodes, but each each of them is equipped
/// with a small amount of cores. In such a case, the combining degree of H-Synch may be restricted. Thus, creating a 
/// fictitious topology with restricted number of NUMA nodes gives much better performance. The user usually overides 
/// HSYNCH_DEFAULT_NUMA_POLICY by setting the '-n' argument in the executable of the benchmarks.
#define HSYNCH_DEFAULT_NUMA_POLICY 0

/// @brief HalfHSynchNode should not be directly used by the user.
/// It is internally used for proper alignment of the HSynchNode struct.
typedef struct HalfHSynchNode {
    struct HalfHSynchNode *next;
    ArgVal arg_ret;
    uint32_t pid;
    uint32_t locked;
    uint32_t completed;
} HalfHSynchNode;

/// @brief HSynchNode stores the data of an announced request.
typedef struct HSynchNode {
    /// @brief Pointer to the next request that has been announced.
    struct HSynchNode *next;
    /// @brief This variable stores the argument of the request and the return value after the request is applied.
    ArgVal arg_ret;
    /// @brief The pid of the thread that announced this request.
    uint32_t pid;
    /// @brief Whenever it is equal to false, the thread is the combiner; otherwise the thread waits until a combiner apply its request.
    uint32_t locked;
    /// @brief If true, the request is applied and the thread returns its return value.
    uint32_t completed;
    /// @brief Padding space.
    char align[PAD_CACHE(sizeof(HalfHSynchNode))];
} HSynchNode;

/// @brief HSynchNodePtr is a struct for padding pointers to nodes of requests.
typedef union HSynchNodePtr {
    volatile HSynchNode *ptr;
    char pad[CACHE_LINE_SIZE];
} HSynchNodePtr;

/// @brief HSynchThreadState stores each thread's local state for a single instance of HSynch.
/// For each instance of HSynch, a discrete instance of HSynchThreadState should be used.
typedef struct HSynchThreadState {
    /// @brief pointer to an empty request that would be used for announcing future requests.
    HSynchNode *next_node;
} HSynchThreadState;

///  @brief HSynchStruct stores the state of an instance of the a HSynch combining object.
/// HSynchStruct should be initialized using the HSynchStructInit function.
typedef struct HSynchStruct {
    /// @brief A CLH lock is used giving to the threads of each Numa node exclusive access to the object.
    CLHLockStruct *central_lock CACHE_ALIGN;
    /// @brief A tail to the list of announced requests.
    HSynchNodePtr *Tail CACHE_ALIGN;
#ifdef DEBUG
    volatile uint64_t counter CACHE_ALIGN;
    volatile int rounds;
#endif
    /// @brief Pointer to pools of nodes used by threads in order to announce their requests.
    /// HSynch maintains a discrete pool for each Numa node.
    HSynchNode **nodes CACHE_ALIGN;
    /// @brief Used for constructing the Numa topology.
    int32_t *node_indexes;
    /// @brief The number of threads that will use the HSynch combining object.
    uint32_t nthreads;
    /// @brief The size in terms of processing elements that each Numa node has.
    uint32_t numa_node_size;
    /// @brief The number of Numa nodes.
    uint32_t numa_nodes;
    /// @brief The numa policy that the system follows.
    bool numa_policy;
} HSynchStruct;

/// @brief This function initializes an instance of the HSynch combining object.
///
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any request by using the HSynchApplyOp function.
///
/// @param l A pointer to an instance of the HSynch combining object.
/// @param nthreads The number of threads that will use the HSynch combining object.
/// @param numa_regions The number of Numa nodes (which may differ with the actual hw numa nodes) that H-Synch should consider.
/// In case that numa_nodes is equal to HSYNCH_DEFAULT_NUMA_POLICY, the number of Numa nodes provided by the HW is used
/// (see more on hsynch.h).
void HSynchStructInit(HSynchStruct *l, uint32_t nthreads, uint32_t numa_regions);

/// @brief This function should be called once before the thread applies any operation to the HSynch combining object.
///
/// @param l A pointer to an instance of the HSynch combining object.
/// @param st_thread A pointer to thread's local state of HSynch.
/// @param pid The pid of the calling thread.
void HSynchThreadStateInit(HSynchStruct *l, HSynchThreadState *st_thread, int pid);

/// @brief This function is called whenever a thread wants to apply an operation to the simulated concurrent object.
///
/// @param l A pointer to an instance of the HSynch combining object.
/// @param st_thread A pointer to thread's local state for a specific instance of HSynch.
/// @param sfunc A serial function that the HSynch instance should execute, while applying requests announced by active threads.
/// @param state A pointer to the state of the simulated object.
/// @param arg The argument of the request that the thread wants to apply.
/// @param pid The pid of the calling thread.
/// @return RetVal The return value of the applied request.
RetVal HSynchApplyOp(HSynchStruct *l, HSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);
#endif
