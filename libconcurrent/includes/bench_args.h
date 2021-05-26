/// @file bench_args.h
/// @brief This file exposes the API of a simple parser for the command-line arguments used by the benchmarks provided by the Synch framework.
/// Examples of usage could be found in almost all the provided benchmarks under the benchmarks directory.
#ifndef _BENCH_ARGS_H_
#define _BENCH_ARGS_H_

#include <stdint.h>

/// @brief BenchArgs stores the values of the command-line arguments used by the benchmarks provided by the Synch framework.
/// BenchArgs should be initialized using the parseArguments function. For the default values, see the config.h file.
typedef struct BenchArgs {
    /// @brief The number of the executed operations per thread. Notice that benchmarks for stacks and queues
    /// execute runs pairs of operations (i.e. pairs of push/pops or pairs of enqueues/dequeues).
    uint64_t runs;
    /// @brief The total number of the executed operations. Notice that benchmarks for stacks and queues
    /// execute total_runs pairs of operations (i.e. pairs of push/pops or pairs of enqueues/dequeues).
    uint64_t total_runs;
    /// @brief The number of posix threads that will be used by the benchmark.
    uint32_t nthreads;
    /// @brief The number of fibers per posix thread that will be used by the benchmark. The default value is equal to 1.
    uint32_t fibers_per_thread;
    /// @brief The  value of maximum local work that each thread executes between two consecutive of the
    /// benchmarked operation. A zero value means there is no work between two consecutive calls.
    /// Large values usually reduce system's contention, i.e. threads perform operations less frequentely.
    /// In constrast, small values (but not zero) increase system's contention. Please avoid to set this value
    /// equal to zero, since some algorithms may produce unreallistically high performan (i.e. long runs
    /// and unrealistic low numbers of cache misses).
    uint32_t max_work;
    /// @brief The number of numa nodes (which may differ with the actual harware numa nodes) that hierarchical algorithms should take account.
    uint32_t numa_nodes;
    /// @brief The lower backoff bound used in the experiment.
    uint16_t backoff_low;
    /// @brief The upper backoff bound used in the experiment.
    uint16_t backoff_high;
} BenchArgs;

/// @brief This function parses the command-line arguments and stores them in an BenchArgs structure.
///
/// @param bench_args A pointer to an BenchArgs structure.
/// @param argc The number of command-line arguments.
/// @param argv An array of argc command-line arguments (i.e. strings) to be parsed.
void parseArguments(BenchArgs *bench_args, int argc, char *argv[]);

#endif
