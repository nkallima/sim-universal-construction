[![check-build](https://github.com/nkallima/sim-universal-construction/actions/workflows/check-build.yml/badge.svg)](https://github.com/nkallima/sim-universal-construction/actions/workflows/check-build.yml) [![validate](https://github.com/nkallima/sim-universal-construction/actions/workflows/validate.yml/badge.svg)](https://github.com/nkallima/sim-universal-construction/actions/workflows/validate.yml) [![codecov](https://codecov.io/gh/nkallima/sim-universal-construction/branch/codecov/graph/badge.svg?token=1V8A6BOABM)](https://codecov.io/gh/nkallima/sim-universal-construction) [![status](https://joss.theoj.org/papers/07bf35ba1bd72c38cc8076fee6864409/status.svg)](https://joss.theoj.org/papers/07bf35ba1bd72c38cc8076fee6864409)

<p align="center">
    <img src="resources/logo_synch.png" alt="The Synch Framework" width="80%">
</p>

# Summary

This is an open-source framework for concurrent data-structures and benchmarks. The provided framework contains a substantial set of concurrent data-structures such as `queues`, `stacks`, `combining-objects`,
`hash-tables`, `locks`, etc. This framework also provides a user-friendly runtime for developing and benchmarking concurrent data-structures. Among other features, this runtime provides functionality for creating threads easily (both Posix and user-level threads), tools for measuring performance, etc. The provided concurrent data-structures and the runtime are highly optimized for contemporary NUMA multiprocessors such as AMD Epyc and Intel Xeon.

The current version of this code is optimized for x86_64 machine architecture, but the code is also successfully tested in other machine architectures, such as ARM-V8 and RISC-V. Some of the benchmarks perform much better in architectures that natively support Fetch&Add instructions (e.g., x86_64, etc.).


# Collection

The Synch framework provides a large set of highly efficient concurrent data-structures, such as combining-objects, concurrent queues and stacks, concurrent hash-tables and locks. The cornerstone of the Synch framework are the combining objects. A Combining object is a concurrent object/data-structure that is able to simulate any other concurrent object, e.g. stacks, queues, atomic counters, barriers, etc. The Synch framework provides the PSim wait-free combining object [2,10], the blocking combining objects CC-Synch, DSM-Synch and H-Synch [1], and the blocking combining object based on the technique presented in [4]. Moreover, the Synch framework provides the Osci blocking, combining technique [3] that achieves good performance using user-level threads.

In terms of concurrent queues, the Synch framework provides the SimQueue [2,10] wait-free queue implementation that is based on the PSim combining object, the CC-Queue, DSM-Queue and H-Queue [1] blocking queue implementations based on the CC-Synch, DSM-Synch and H-Synch combining objects. A blocking queue implementation based on the CLH locks [5,6] and the lock-free implementation presented in [7] are also provided.
Since v2.4.0, the Synch framework provides the LCRQ [11,12] queue implementation. In terms of concurrent stacks, the Synch framework provides the SimStack [2,10] wait-free stack implementation that is based on the PSim combining object, the CC-Stack, DSM-Stack and H-Stack [1] blocking stack implementations based on the CC-Synch, DSM-Synch and H-Synch combining objects. Moreover, the lock-free stack implementation of [8] and the blocking implementation based on the CLH locks [5,6] are provided. The Synch framework also provides concurrent queue and stacks implementations (i.e. OsciQueue and OsciStack implementations) that achieve very high performance using user-level threads [3].

Furthermore, the Synch framework provides a few scalable lock implementations, i.e. the MCS queue-lock presented in [9] and the CLH queue-lock presented in [5,6]. Finally, the Synch framework provides two example-implementations of concurrent hash-tables. More specifically, it provides a simple implementation based on CLH queue-locks [5,6] and an implementation based on the DSM-Synch [1] combining technique.

The following table presents a summary of the concurrent data-structures offered by the Synch framework.
| Concurrent  Object    |                Provided Implementations                           |
| --------------------- | ----------------------------------------------------------------- |
| Combining Objects     | CC-Synch, DSM-Synch and H-Synch [1]                               |
|                       | PSim [2,10]                                                       |
|                       | Osci [3]                                                          |
|                       | Oyama [4]                                                         |
| Concurrent Queues     | CC-Queue, DSM-Queue and H-Queue [1]                               |
|                       | SimQueue [2,10]                                                   |
|                       | OsciQueue [3]                                                     |
|                       | CLH-Queue [5,6]                                                   |
|                       | MS-Queue [7]                                                      |
|                       | LCRQ [11,12]                                                      |
| Concurrent Stacks     | CC-Stack, DSM-Stack and H-Stack [1]                               |
|                       | SimStack [2,10]                                                   |
|                       | OsciStack [3]                                                     |
|                       | CLH-Stack [5,6]                                                   |
|                       | LF-Stack [8]                                                      |
| Locks                 | CLH [5,6]                                                         |
|                       | MCS [9]                                                           |
| Hash Tables           | CLH-Hash [5,6]                                                    |
|                       | A hash-table based on DSM-Synch [1]                               |


# Requirements

- A modern 64-bit machine. Currently, 32-bit architectures are not supported. The current version of this code is optimized for the x86_64 machine architecture, but the code is also successfully tested in other machine architectures, such as ARM-V8 and RISC-V. Some of the benchmarks perform much better in architectures that natively support Fetch&Add instructions (e.g., x86_64, etc.).
- A recent Linux distribution. The Synch environment may also build/run in some other Unix-like systems, (i.e. BSD, etc.). In this case the result is not guaranteed, since the environment is not tested in systems other than Linux.
- As a compiler, gcc of version 4.8 or greater is recommended, but you may also try to use icx or clang.
- Building requires the following development packages:
    - `libatomic`
    - `libnuma`
    - `libpapi` in case that the `SYNCH_TRACK_CPU_COUNTERS` flag is enabled in `libconcurrent/config.h`.
- For building the documentation (i.e. man-pages), `doxygen` is required.


# Configuring, compiling and installing the framework

In the `libconcurrent/config.h` file, the user can configure some basic options for the framework, such as:
- Enable/disable debug mode.
- Support for Numa machines.
- Enable performance statistics, etc.

The provided default configuration should work well in many cases. However, the default configuration may not provide the best performance. For getting the best performance, modifying the `libconcurrent/config.h` may be needed (see more on Performance/Optimizations Section).

In case that you want to compile the library that provides all the implemented concurrent algorithms just execute `make` in the root directory of the source files. This step is necessary in case that you want to run benchmarks. However, some extra make options are provided in case the user wants to compile the framework with other than system's default compiler, clean the binary files, etc. The following table provides the list with all the available make options.

|     Command             |                                Description                                                                    |
| ----------------------- | ------------------------------------------------------------------------------------------------------------- |
|  `make`                 |  Auto-detects the current architecture and compiles the source-code for it (this should work for most users). |
|  `make CC=cc`           |  Compiles the source-code for the current architecture using the `cc` compiler.                               |
|  `make clang`           |  Compiles the source-code using the clang compiler.                                                           |
|  `make icx`             |  Compiles the source-code using the Intel icx compiler.                                                       |
|  `make unknown`         |  Compiles the source-code for architectures other than X86_64, e.g. RISC-V, ARM, etc.                         |
|  `make clean`           |  Cleaning-up all the binary files.                                                                            |
|  `make docs`            |  Creating the documentation (i.e. man-pages).                                                                 |
|  `make install`         |  Installing the framework on the default location (i.e. `/opt/Synch/`).                                       |
|  `make install DIR=dir` |  Installing the framework on the `dir/Synch/` location.                                                       |
|  `make uninstall`       |  Uninstalling the framework.                                                                                  |

For building the documentation (i.e. man-pages), the user should execute `make docs`. Notice that for building the documentation the system should be equipped with `doxygen` documentation tool.

For installing the framework, the user should execute `make install`. In this case, the framework will be installed in the default location which is `/opt/Synch/`. Notice that in this case, the user should have write access on the `/opt` directory or sudo access. The `make install DIR=dir` command installs the framework in the `dir/Synch` path, while the `make uninstall` uninstalls the framework. For accessing the man pages, the user should manually setup the `MANPATH` environmental variable appropriately (e.g. `export MANPATH=$MANPATH:/opt/Synch/docs/man`).


# Running Benchmarks

For running benchmarks use the `bench.sh` script file that is provided in the main directory of this source tree.

Example usage: `./bench.sh FILE.run OPTION1=NUM1 OPTION2=NUM2 ...`

Each benchmark reports the time that needs to be completed, the average throughput of operations performed and some performance statistics if `DEBUG` option is enabled during framework build. The `bench.sh` script measures the strong scaling of the benchmark that is executed.

The following options are available:

|     Option              |                       Description                                                     |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
|  `-t`, `--max_threads`  |  set the maximum number number of POSIX threads to be used in the last set of iterations of the benchmark, default is the number of system cores |
|  `-s`, `--step`         |  set the step (extra number of threads to be used) in each set of iterations of the benchmark, default is number of processors/8 or 1            |
|  `-f`, `--fibers`       |  set the number of user-level threads per posix thread                                                                                           |
|  `-r`, `--repeat`       |  set the total number of operations executed by the benchmark, default is 1000000                                                                |
|  `-i`, `--iterations`   |  set the number of times that the benchmark should be executed, default is 10                                                                    |
|  `-w`, `--workload`     |  set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the benchmarked object), default is 64               |
|  `-l`, `--list`         |  displays the list of the available benchmarks                                                                                                   |
|  `-n`, `--numa_nodes`   |  set the number of numa nodes (which may differ with the actual hw numa nodes) that hierarchical algorithms should take account                  |
|  `-b`, `--backoff`, `--backoff_high` |  set an upper backoff bound for lock-free and Sim-based algorithms                                                                  |
|  `-bl`, `--backoff_low` |  set a lower backoff bound (only for msqueuebench, lfstackbench and lfuobjectbench benchmarks)                                                                  |
|  `-h`, `--help`         |  displays this help and exits                                                                                                                    |

The framework provides the `validate.sh` validation/smoke script. The `validate.sh` script compiles the sources in `DEBUG` mode and runs a big set of benchmarks with various numbers of threads. After running each of the benchmarks, the script evaluates the `DEBUG` output and in case of success it prints `PASS`. In case of a failure, the script simply prints `FAIL`. In order to see all the available options of the validation/smoke script, execute `validate.sh -h`. Given that the `validate.sh` validation/smoke script depends on binaries that are compiled in `DEBUG` mode, it is not installed while executing `make install`. The following image shows the execution and the default behavior of `validate.sh`.

![](resources/validate_example.gif)

The framework provides another simple fast smoke test: `./run_all.sh`. This will quickly run all available benchmarks with default options and store the results in the `results.txt` file.


# Performance/Optimizations

Getting the best performance from the provided benchmarks is not always an easy task. For getting the best performance, some modifications in Makefiles may be needed (compiler flags, etc.). Important parameters for the benchmarks and/or library are placed in the `libconcurrent/config.h` file. A useful guide to consider in order to get better performance in a modern multiprocessor follows.

- In case that the target machine is a NUMA machine make sure `SYNCH_NUMA_SUPPORT` is enabled in `libconcurrent/config.h`. Usually, when this option is enabled, it gives much better performance in NUMA machines. However, in some older machines this option may induce performance overheads.
- Whenever the `SYNCH_NUMA_SUPPORT` option is enabled, the runtime will detect the system's number of NUMA nodes and will setup the environment appropriately. However, significant performance benefits have been observed by manually setting-up the number of NUMA nodes manually (see the `--numa_nodes` option). For example, the performance of the H-Synch family algorithms on an AMD EPYC machine consisting of 2x EPYC 7501 processors (i.e., 128 hardware threads) is much better by setting `--numa_nodes` equal to `2`. Notice that the runtime successfully reports that the available NUMA nodes are `8`, but this value is not optimal for H-Synch in this configuration. An experimental analysis for different values of `--numa_nodes` may be needed.
- Check the performance impact of the `SYNCH_COMPACT_ALLOCATION` option in `libconcurrent/config.h`. In modern AMD multiprocessors (i.e., equipped with EPYC processors) this option gives tremendous performance boost. In contrast to AMD processors, this option introduces serious performance overheads in Intel Xeon processors. Thus, a careful experimental analysis is needed in order to show the possible benefits of this option.
- Check the cache line size (`CACHE_LINE_SIZE` and `S_CACHE_LINE` options in includes/system.h). These options greatly affect the performance in all modern processors. Most Intel machines behave better with `CACHE_LINE_SIZE` equal or greater than `128`, while most modern AMD machine achieve better performance with a value equal to `64`. Notice that `CACHE_LINE_SIZE` and `S_CACHE_LINE` depend on the `SYNCH_COMPACT_ALLOCATION` option (see includes/system.h).
- Use backoff if it is available. Many of the provided algorithms could use backoff in order to provide better performance (e.g., sim, LF-Stack, MS-Queue, SimQueue, SimStack, etc.). In this case, it is of crucial importance to use `-b` (and in some cases `-bl` arguments) in order to get the best performance. 
- Ensure that you are using a recent gcc-compatible compiler, e.g. a `gcc` compiler of version `7.0` or greater is highly recommended.
- Check the performance impact of the different available compiler optimizations. In most cases, gcc's `-Ofast` option gives the best performance. In addition, some algorithms (i.e., sim, osci, simstack, oscistack, simqueue and osciqueue) benefit by enabling the `-mavx` option (in case that AVX instructions are supported by the hardware).
- Check if system oversubscription with user-level fibers enhances the performance. Many algorithms (i.e., the Sim and Osci families of algorithms) show tremendous performance boost by using oversubscription with user-level threads [3]. In this case, use the `--fibers` option.

## Expected performance

The expected performance of the Synch framework is discussed in the [PERFORMANCE.md](PERFORMANCE.md) file.

# Memory reclamation (stacks and queues)

The Synch framework provides a pool mechanism (see `includes/pool.h`) that efficiently allocates and de-allocates memory for the provided concurrent stack and queue implementations. The allocation mechanism of this pool implementation is low-overhead.  All the provided stack and queue implementations use the functionality of this pool mechanism. In order to support memory reclamation in a safe manner, a concurrent object should guarantee that each memory object that is going to de-allocated should be accessed only by the thread that is going to free it. Generally, de-allocating and thus reclaiming memory is easy in many blocking objects, since there is a lock that protects the de-allocated memory object. Currently, the Synch framework supports memory reclamation for the following concurrent stack and queue implementations:
- Concurrent Queues:
    - CC-Queue, DSM-Queue and H-Queue [1]
    - OsciQueue [3]
    - CLH-Queue [5,6]
- Concurrent Stacks:
    - CC-Stack, DSM-Stack and H-Stack [1]
    - OsciStack [3]
    - CLH-Stack [5,6]
    - SimStack [2,10] (since v2.4.0)

Note that de-allocating and thus recycling memory in lock-free and wait-free objects is not an easy task. Since v2.4.0, SimStack supports memory reclamation using the functionality of `pool.h` and a technique that is similar to that presented by Blelloch and Weiin in [13]. Notice that the MS-Queue [7], LCRQ [11,12] queue implementations and the LF-Stack [8] stack implementation support memory reclamation through hazard-pointers. However, the current version of the Synch framework does not provide any implementation of hazard-pointers. In case that a user wants to use memory reclamation in these objects, a custom hazard-pointers implementation should be integrated in the environment.

By default, memory-reclamation is enabled. In case that there is need to disable memory reclamation, the `SYNCH_POOL_NODE_RECYCLING_DISABLE` option should be enabled in `config.h`.

The following table shows the memory reclamation characteristics of the provided stack and queues implementations.

| Concurrent  Object    |        Provided Implementations           | Memory Reclamation                        |
| --------------------- | ----------------------------------------- | ----------------------------------------- |
| Concurrent Queues     | CC-Queue, DSM-Queue and H-Queue [1]       | Supported                                 |
|                       | SimQueue [2,10]                           | Not supported                             |
|                       | OsciQueue [3]                             | Supported                                 |
|                       | CLH-Queue [5,6]                           | Supported                                 |
|                       | MS-Queue [7]                              | Hazard Pointers (not provided by Synch)   |
|                       | LCRQ [11,12]                              | Hazard Pointers (not provided by Synch)   |
| Concurrent Stacks     | CC-Stack, DSM-Stack and H-Stack [1]       | Supported                                 |
|                       | SimStack [2,10]                           | Supported (since v2.4.0)                  |
|                       | OsciStack [3]                             | Supported                                 |
|                       | CLH-Stack [5,6]                           | Supported                                 |
|                       | LF-Stack [8]                              | Hazard Pointers (not provided by Synch)   |


## Memory reclamation limitations

In the current design of the reclamation mechanism, each thread uses a single private pool for reclaiming memory. In a producer-consumer scenario where a set of threads performs only enqueue operations (or push operations in case of stacks) and all other threads perform dequeue operations (or pop operations in case of stacks), insufficient memory reclamation is performed since each memory pool is only accessible by the thread that owns it. We aim to improve this in future versions of the Synch framework.


# API documentation

A complete API documentation is provided in [https://nkallima.github.io/sim-universal-construction/index.html](https://nkallima.github.io/sim-universal-construction/index.html).

# Code example for a simple benchmark
We now describe a very simple example-benchmark that uses the Application Programming Interface (API) of the provided runtime. This simple benchmark measures the performance of Fetch&Add instructions in multi-core machines. The purpose of this simple benchmark is to measure the performance of Fetch&Add implementations (hardware or software).

```c
#include <stdio.h>
#include <stdint.h>

#include <primitives.h>
#include <threadtools.h>
#include <barrier.h>

#define N_THREADS 10
#define RUNS      1000000

volatile int64_t object CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;
SynchBarrier bar CACHE_ALIGN;

inline static void *Execute(void *Arg) {
    long i, id = (long)Arg;

    synchBarrierWait(&bar);
    if (id == 0) d1 = synchGetTimeMillis();

    for (i = 0; i < RUNS; i++)
        synchFAA64(&object, 1);

    synchBarrierWait(&bar);
    if (id == 0) d2 = synchGetTimeMillis();

    return NULL;
}

int main(int argc, char *argv[]) {
    object = 1;

    synchBarrierSet(&bar, N_THREADS);
    synchStartThreadsN(N_THREADS, Execute, SYNCH_DONT_USE_UTHREADS);
    synchJoinThreadsN(N_THREADS - 1);

    printf("time: %ld (ms)\tthroughput: %.2f (millions ops/sec)\n", 
           (d2 - d1), RUNS * N_THREADS / (1000.0 * (d2 - d1)));

    return 0;
}
```

This example-benchmark creates `N_THREADS`, where each of them executes `RUNS` Fetch&Add operations in a shared 64-bit integer. At the end of the benchmark the throughput (i.e. Fetch&Add operations per second) is calculated. By seting varous values for `N_THREADS`, this benchmark is able to measure strong scaling.

<<<<<<< HEAD
The `StartThreadsN` function (provided by the API defined in `threadtools.h`) in main, creates `N_THREADS` threads and each of the executes the `Execute` function declared in the same file. The `SYNCH_DONT_USE_UTHREADS` argument imposes `StartThreadsN` to create only Posix threads; in case that the user sets the corresponding fibers argument to `M` > 0, then `StartThreadsN` will create `N_THREADS` Posix threads and each of them will create `M` user-level (i.e. fiber) threads. The `JoinThreadsN` function (also provided by `threadtools. h`) waits until all Posix and fiber (if any) threads finish the execution of the `Execute` function. The Fetch&Add instruction on 64-bit integers is performed by the `FAA64` function provided by the API of `primitives.h`.
=======
The `synchStartThreadsN` function (provided by the API defined in `threadtools.h`) in main, creates `N_THREADS` threads and each of the executes the `Execute` function declared in the same file. The `_DONT_USE_UTHREADS_` argument imposes `synchStartThreadsN` to create only Posix threads; in case that the user sets the corresponding fibers argument to `M` > 0, then `StartThreadsN` will create `N_THREADS` Posix threads and each of them will create `M` user-level (i.e. fiber) threads. The `synchJoinThreadsN` function (also provided by `threadtools. h`) waits until all Posix and fiber (if any) threads finish the execution of the `Execute` function. The Fetch&Add instruction on 64-bit integers is performed by the `synchFAA64` function provided by the API of `primitives.h`.
>>>>>>> 37d9803a0fb9444e413dcae23a8cc4a84ab24506

The threads executing the `Execute` function use the `SynchBarrier` re-entrant barrier object for simultaneously starting to perform Fetch&Add instructions on the shared variable `object`. This barrier is also re-used before the end of the `Execute` function in order to allow thread with `id = 0` to measure the amount of time that the benchmark needed for completion. The `synchBarrierSet` function in `main` initializes the `SynchBarrier` object. The `synchBarrierSet` takes as an argument a pointer to the barrier object and the number of threads `N_THREADS` that are going to use it. Both `synchBarrierSet` and `synchBarrierWait` are provided by the API of `barrier.h`

At the end of the benchmark, `main` calculates and prints the average throughput of Fetch&Add operations per second achieved by the benchmark.


# If you want to cite us

```latex
@misc{SynchFramework,
  title={{Synch: A framework for concurrent data-structures and benchmarks. https://github.com/nkallima/sim-universal-construction}},
  author={Kallimanis, Nikolaos D.},
  url={{https://github.com/nkallima/sim-universal-construction}}
}
```

# Releases

An extensive list of the recent releases and their features is provided at [https://github.com/nkallima/sim-universal-construction/releases](https://github.com/nkallima/sim-universal-construction/releases).

# License

The Synch framework is provided under the [LGPL-2.1 License](https://github.com/nkallima/sim-universal-construction/blob/main/LICENSE).

# Code of conduct

[Code of conduct](https://github.com/nkallima/sim-universal-construction/blob/main/.github/CODE_OF_CONDUCT.md).

# References

[1]. Panagiota Fatourou, and Nikolaos D. Kallimanis. "Revisiting the combining synchronization technique". ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.

[2]. Panagiota Fatourou, and Nikolaos D. Kallimanis. "A highly-efficient wait-free universal construction". Proceedings of the twenty-third annual ACM symposium on Parallelism in algorithms and architectures (SPAA), 2011.

[3]. Panagiota Fatourou, and Nikolaos D. Kallimanis. "Lock Oscillation: Boosting the Performance of Concurrent Data Structures" Proceedings of the 21st International Conference on Principles of Distributed Systems (Opodis), 2017.

[4]. Yoshihiro Oyama, Kenjiro Taura, and Akinori Yonezawa. "Executing parallel programs with synchronization bottlenecks efficiently". Proceedings of the International Workshop on Parallel and Distributed Computing for Symbolic and Irregular Applications. Vol. 16. 1999.

[5]. Travis S. Craig. "Building FIFO and priority-queueing spin locks from atomic swap". Technical Report TR 93-02-02, Department of Computer Science, University of Washington, February 1993.

[6]. Peter Magnusson, Anders Landin, and Erik Hagersten. "Queue locks on cache coherent multiprocessors". Parallel Processing Symposium, 1994. Proceedings., Eighth International. IEEE, 1994.
    
[7]. Maged M. Michael, and Michael L. Scott. "Simple, fast, and practical non-blocking and blocking concurrent queue algorithms". Proceedings of the fifteenth annual ACM symposium on Principles of distributed computing. ACM, 1996.
    
[8]. R. Kent Treiber. "Systems programming: Coping with parallelism". International Business Machines Incorporated, Thomas J. Watson Research Center, 1986.

[9]. John M. Mellor-Crummey, and Michael L. Scott. "Algorithms for scalable synchronization on shared-memory multiprocessors". ACM Transactions on Computer Systems (TOCS) 9.1 (1991): 21-65.

[10]. Panagiota Fatourou, and Nikolaos D. Kallimanis. "Highly-efficient wait-free synchronization". Theory of Computing Systems 55.3 (2014): 475-520.

[11]. Adam Morrison, and Yehuda Afek. "Fast concurrent queues for x86 processors". Proceedings of the 18th ACM SIGPLAN symposium on Principles and practice of parallel programming. 2013.

[12]. Adam Morrison, and Yehuda Afek. Source code for LCRQ. http://mcg.cs.tau.ac.il/projects/lcrq.

[13]. Guy E. Blelloch, and Yuanhao Wei. "Brief Announcement: Concurrent Fixed-Size Allocation and Free in Constant Time." 34th International Symposium on Distributed Computing (DISC 2020). Schloss Dagstuhl-Leibniz-Zentrum für Informatik, 2020.

# Contact

For any further information, please do not hesitate to
send an email at nkallima (at) ics.forth.gr. Feedback is always valuable.
