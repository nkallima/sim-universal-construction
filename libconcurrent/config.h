#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

// Definition: MAX_WORK
// --------------------
// Define the maximum local work that each thread executes between two calls of some
// simulated shared object's operation. A zero value means no work between two calls.
// The exact value depends on the speed of processing cores. Try not to use big values
// (avoiding slow contention) or not to use small values (avoiding long runs and
// unrealistic cache misses ratios).
#ifndef MAX_WORK
#    define MAX_WORK               64
#endif

// definition: RUNS
// ----------------
// Define the total number of the calls of object's  operations that will be executed.
#define RUNS                       1000000

// Definition: DEBUG
// -----------------
// Enable this definition in case you want to debug some parts of the code or to get some
// useful performance statistics. This usually leads to performance loss. See README.txt
// for more details.
//#define DEBUG

// Definition: DISABLE_BACKOFF
// ---------------------------
// By defining this, any backoff scheme used by any algorithm is disabled. Be careful,
// upper an lower bounds must be used in benchmark scripts, but they are ignored.
//#define DISABLE_BACKOFF

#define Object                     int64_t

// Definition: RetVal
// ------------------
// Define the type of the return value that simulated atomic objects return. Be careful,
// it is assumed that the target architecture is able to atomically read/write this type.
// If this type is of 32 or 64 bits could be atomically read or written by most machine
// architectures. However, a value of 128 bits or more may not be supported(in most cases
// x86_64 supports types of 128 bits).
#define RetVal                     int64_t

// Definition: ArgVal
// ------------------
// Define the type of the argument value of atomic objects. All atomic objects have same
// argument types. In case that you 'd like to use different argument values in each
// atomic object, redefine it in object's source file.
#define ArgVal                     int64_t

#define NUMA_SUPPORT

// Definition: SYNCH_COMPACT_ALLOCATION
// ------------------------------------
// This definition enables some optimizations on memory allocation that seems to greately
// improve the performance on AMD Epyc multiprocessors. This flag seems to double the
// performance in CC-Synch and H-Synch algorithms.
#define SYNCH_COMPACT_ALLOCATION

// Definition: POOL_NODE_RECYCLING_DISABLE
// ---------------------------------------
// This definition disables node recycling in the concurrent stack and queue
// implementations that support memory reclamation. This may have negative impact in
// performance. By default, this flag is enabled.
//#define POOL_NODE_RECYCLING_DISABLE

// Definition: _TRACK_CPU_COUNTERS
// -------------------------------
// By enabling this definition, the Performance Application Programming Interface (PAPI)
// is used for getting performance counters during the execution of benchmarks. In this
// case, the PAPI library should be install and appropriately configured.
//#define _TRACK_CPU_COUNTERS

//#define _EMULATE_FAA_
//#define _EMULATE_SWAP_

#endif

