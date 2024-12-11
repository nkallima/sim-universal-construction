/// @file config.h
/// @author Nikolaos D. Kallimanis (nkallima@gmail.com)
/// @brief This file provides important parameters and constants for the benchmarks and the library.
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

/// @brief Defines the default value of maximum local work that each thread executes between two consecutive of the
/// benchmarked operation. A zero value means there is no work between two consecutive calls.
/// Large values usually reduce system's contention, i.e. threads perform operations less frequently.
/// In contrast, small values (but not zero) increase system's contention. Please avoid to set this value
/// equal to zero, since some algorithms may produce unrealistically high performance (i.e. long runs
/// and unrealistic low numbers of cache misses).
/// Default value is 64.
#ifndef SYNCH_MAX_WORK
#    define SYNCH_MAX_WORK         64
#endif

/// @brief Defines the default total number of the executed operations. Notice that benchmarks for stacks and queues
/// execute SYNCH_RUNS pairs of operations (i.e. pairs of push/pops or pairs of enqueues/dequeues).
/// Default value is 1000000.
#define SYNCH_RUNS                 1000000

/// @brief Define DEBUG, in case you want to debug some parts of the code or to get some  useful performance statistics.
/// Note that the validation.sh script enables this definition by default. In some cases, this may introduces
/// some performance loses. Thus, in case you want to perform benchmarking keeps this undefined.
/// By default, this flag is disabled.
//#define DEBUG

/// @brief This definition disables backoff in all algorithms that are using it for reducing system's contention.
/// By default, this flag is disabled.
//#define SYNCH_DISABLE_BACKOFF

#define Object                     int64_t

/// @brief Defines the type of the return value that most operations in the provided benchmarks return.
/// For instance, the return values of all pop and dequeue operations (implemented by the provided stacks and queue
/// objects) return a value of type RetVal. It is assumed that the target architecture is able to atomically read/write
/// this type If this type is of 32 or 64 bits could be atomically read or written by most machine architectures.
/// However, a value of 128 bits or more may not be supported (in most cases x86_64 supports types of 128 bits).
/// By default, this definition is equal to int64_t
#define RetVal                     int64_t

/// @brief Defines the type of the argument value of atomic operations provided by the implemented concurrent objects.
/// For example, all push and enqueue operations (implemented by the provided stacks and queue objects) get an
/// argument of type ArgVal. It is assumed that the target architecture is able to atomically read/write this type.
/// If this type is of 32 or 64 bits could be atomically read or written by most machine architectures.
/// However, a value of 128 bits or more may not be supported (in most cases x86_64 supports types of 128 bits).
/// By default, this definition is equal to int64_t
#define ArgVal                     int64_t

/// @brief Whenever the `SYNCH_NUMA_SUPPORT` option is enabled, the runtime will detect the system's number of NUMA nodes
/// and will setup the environment appropriately. However, significant performance benefits have been observed by
/// manually setting-up the number of NUMA nodes manually (see the `--numa_nodes` option). For example, the performance
/// of the H-Synch family algorithms on an AMD EPYC machine consisting of 2x EPYC 7501 processors (i.e., 128 hardware threads)
/// is much better by setting `--numa_nodes` equal to `2`. Note that the runtime successfully reports that the available
/// NUMA nodes are `8`, but this value is not optimal for H-Synch in this configuration. An experimental analysis for
/// different values of `--numa_nodes` may be needed.
#define SYNCH_NUMA_SUPPORT

/// @brief This definition enables some optimizations on memory allocation that seems to greatly improve the performance
/// on AMD Epyc multiprocessors. This flag seems to double the performance in CC-Synch and H-Synch algorithms.
/// In contrast to AMD processors, this option introduces serious performance overheads in Intel Xeon processors. 
/// Thus, a careful experimental analysis is needed in order to show the possible benefits of this option.
/// By default, this flag is enabled.
#define SYNCH_COMPACT_ALLOCATION

/// @brief This definition disables node recycling in the concurrent stack and queue implementations that support memory
/// reclamation. This may have negative impact in performance in some case. More on this on the README.md file.
/// By default, this flag is disabled.
//#define SYNCH_POOL_NODE_RECYCLING_DISABLE

/// @brief By enabling this definition, the Performance Application Programming Interface (PAPI library) is used for
/// getting performance counters during the execution of benchmarks. In this case, the PAPI library (i.e. libpapi)
/// should be install and appropriately configured.
/// By default, this flag is disabled.
//#define SYNCH_TRACK_CPU_COUNTERS

/// @brief By enabling this definition, synchFAA32 and synchFAA64 operations will be simulated using synchCAS32 and
/// synchCAS64 operations respectively.
//#define SYNCH_EMULATE_FAA

/// @brief By enabling this definition, synchSWAP operations will be simulated using synchCAS operations.
//#define SYNCH_EMULATE_SWAP

#endif
