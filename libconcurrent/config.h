#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <unistd.h>

// Definition: USE_CPUS
// --------------------
// Define the number of processing cores that your computation
// system offers or the maximum number of cores that you want to use.
#if (USE_CPUS + 0) == 0
#   undef USE_CPUS
#endif
#ifndef USE_CPUS
#    define USE_CPUS               128
#endif

// Definition: N_THREADS
// ---------------------
// Define the number of threads that you like to run the provided 
// experiments. In case N_THREADS > USE_CPUS, two or more threads 
// may run in the same processing core.
#ifndef N_THREADS
#    define N_THREADS              128
#endif


#if (FIBERS_PER_THREAD + 0) == 0
#   undef FIBERS_PER_THREAD
#endif

#ifndef FIBERS_PER_THREAD
#   if N_THREADS < USE_CPUS
#       define FIBERS_PER_THREAD   1
#   else
#       define FIBERS_PER_THREAD   (N_THREADS/USE_CPUS)
#   endif
#endif

// Definition: MAX_WORK
// --------------------
// Define the maximum local work that each thread executes 
// between two calls of some simulated shared object's
// operation. A zero value means no work between two calls.
// The exact value depends on the speed of processing cores.
// Try not to use big values (avoiding slow contention)
// or not to use small values (avoiding long runs and
// unrealistic cache misses ratios).
#ifndef MAX_WORK
#    define MAX_WORK               512
#endif

// definition: RUNS
// ----------------
// Define the total number of the calls of object's 
// operations that will be executed.
#define RUNS                       (100000000 / N_THREADS)

// Definition: DEBUG
// -----------------
// Enable this definition in case you want to debug some
// parts of the code or to get some useful performance 
// statistics. This usually leads to performance loss.
// See README.txt for more details.
//#define DEBUG

// Definition OBJECT_SIZE
// ----------------------
// This definition is only used in lfobject.c, simopt.c
// and luobject.c experiments. In any other case it is
// ignored. Its default value is 1. It is used for simulating
// of an atomic array of Fetch&Multiply objects with
// OBJECT_SIZE elements. All elements are updated simultaneously.
#ifndef OBJECT_SIZE
#    define OBJECT_SIZE            1
#endif

// Definition: DISABLE_BACKOFF
// ---------------------------
// By defining this, any backoff scheme used by any algorithm
// is disabled. Be careful, upper an lower bounds must be
// used in benchmark scripts, but they are ignored.
//#define DISABLE_BACKOFF


#define Object                     int64_t

// Definition: RetVal
// ------------------
// Define the type of the return value that simulated 
// atomic objects return. Be careful, it is assumed 
// that the target architecture is able to atomically
// read/write this type.
// If this type is of 32 or 64 bits could be atomically 
// read or written by most machine architectures.
// However, a value of 128 bits or more may not be supported
// (in most cases i.e. x86_64 supports types of 128 bits).
#define RetVal                     int64_t

// Definition: ArgVal
// ------------------
// Define the type of the argument value of atomic objects.
// All atomic objects have same argument types. In case
// that you 'd like to use different argument values in each
// atomic object, redefine it in object's source file.
#define ArgVal                     int64_t

#define NUMA_SUPPORT

// Definition: _TRACK_CPU_COUNTERS
// -------------------------------
// By enabling this definition, the Performance Application Programming Interface (PAPI)
// is used for getting performance counters during the execution of benchmarks.
// In this case, the PAPI library should be install and appropriately configured.
// IF ENABLE, DO NOT FORGET TO ADD -lpapi OPTION in LDLIBS VARIABLE IN Makefile.generic!
//#define _TRACK_CPU_COUNTERS

//#define _EMULATE_FAA_
//#define _EMULATE_SWAP_

#endif

