# Summary

This is a collection of concurrent shared data structures, such as queue spin-locks, concurrent stacks,
concurrent queues, concurrent hash-tables, etc. Some of these concurrent data-structures are wait-free, 
while some others are lock-free or blocking. This repository also provides a large collection of benchmarks
that exercise the provided data-structure implementations.

The current version of this code is optimized for x86_64 machine architecture, but the code is also
successfully tested in other machine architectures, such as ARM-V8 and RISC-V. 
Some of the benchmarks perform much better in architectures that natively support Fetch&Add
instructions (e.g., x86_64, etc.).

As a compiler, gcc is recommended, but you may also try to use icc or clang.
For compiling the benchmarks, it is highly recommended to use gcc of version 4.3.0 or greater.
Building requires the `libnuma` development package.
For getting the best performance, some modifications in Makefiles may be needed (compiler flags, etc.).
Important parameters for the benchmarks and/or library are placed in the config.h file.


# Running Benchmarks

For running benchmarks use the bench.sh script file that is provided in the main directory of this source tree.

Example usage: `./bench.sh FILE.run OPTION1=NUM1 OPTION2=NUM2 ...`

The following options are available:

|     Option              |                       Description                                                     |
| ----------------------- | ------------------------------------------------------------------------------------- |
|  `-t`, `--max_threads`  |  set the maximum number number of POSIX threads to be used in the last set of iterations of the benchmark, default is the number of system cores |
|  `-s`, `--step`         |  set the step (extra number of threads to be used) in each set of iterations of the benchmark, default is number of processors/8 or 1 |
|  `-f`, `--fibers`       |  set the number of user-level threads per posix thread                                |
|  `-r`, `--repeat`       |  set the number of times that the benchmark should be executed, default is 10 times   |
|  `-w`, `--workload`     |  set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the benchmarked object), default is 64 |
|  `-l`, `--list`         |  displays the list of the available benchmarks                                        |
|  `-n`, `numa_nodes`     |  set the number of numa nodes (which may differ with the actual hw numa nodes) that hierarchical algorithms should take account |
|  `-b`, `--backoff`, `--backoff_high` |  set an upper backoff bound for lock-free and Sim-based algorithms       |
|  `-bl`, `--backoff_low` |  set a lower backoff bound (only for msqueue, lfstack and lfuobject benchmarks)       |
|  `-h`, `--help`         |  displays this help and exits                                                         |

In order to perform smoke test: `./run_all.sh`. This will quickly run all available benchmarks with default options
and store the results in the `results.txt` file.

# Performance/Optimizations

Getting the best performance from the provided benchmarks is not always an easy task. Below, it follows a short-list of steps that a user could follow in order to get better performance in a modern multiprocessor.

- In case that the target machine is a NUMA machine make sure `NUMA_SUPPORT` is enabled in config.h. Usually, when this option is enabled, it gives much better performance in NUMA machines. However, in some older machines this option may induce performance overheads.
- Whenever the `NUMA_SUPPORT` option is enabled, the runtime will detect the system’s number of NUMA nodes and will setup the environment appropriately. However, significant performance benefits have been observed by manually setting-up the number of NUMA nodes manually (see the `--numa_nodes` option). For example, the performance of the H-Synch family algorithms on an AMD EPYC machine consisting of 2x EPYC 7501 processors (i.e., 128 hardware threads) is much better by setting `--numa_nodes` equal to `2`. Notice that the runtime successfully reports that the available NUMA nodes are `8`, but this value is not optimal for H-Synch in this configuration. An experimental analysis for different values of `--numa_nodes` may be needed.
- Check the performance impact of the `SYNCH_COMPACT_ALLOCATION` option in config.h. In modern AMD multiprocessors (i.e., equipped with EPYC processors) this option gives tremendous performance boost. In contrast to AMD processors, this option introduces serious performance overheads in Intel Xeon processors. Thus, a careful experimental analysis should be performed before deciding to enable or not this option.
- Check the cache line size (`CACHE_LINE_SIZE` and `S_CACHE_LINE` options in includes/system.h). These options greatly affect the performance in all modern processors. Most Intel machines behave better with `CACHE_LINE_SIZE` equal or greater than `128`, while most modern AMD machine achieve better performance with a value equal to `64`. Notice that `CACHE_LINE_SIZE` and `S_CACHE_LINE` depend on the `SYNCH_COMPACT_ALLOCATION` option (see includes/system.h).
- Use backoff if it is available. Many of the provided algorithms could use backoff in order to provide better performance (e.g., sim, lfstack, msqueue, simqueue, simstack, etc.). In this case, it is of crucial importance to use `-b` (and in some cases `-bl` arguments) in order to get the best performance. 
- Ensure that you are using a recent gcc-compatible compiler, e.g. a `gcc` compiler of version `7.0` or greater is highly recommended.
- Check the performance impact of the different available compiler optimizations. In most cases, gcc's `-Ofast` option gives the best performance. In addition, some algorithms (i.e., sim, osci, simstack, oscistack, simqueue and osciqueue) benefit by enabling the `-mavx` option (in case that AVX instructions are supported by the hardware).
- Check if system oversubscription with user-level fibers enhances the performance. Many algorithms (i.e., the Sim and Osci families of algorithms) show tremendous performance boost by using oversubscription and user-level threads [3]. In this case, use the `--fibers` option.


# Collection

The current version of this library provides the following concurrent data-structures implementations:

### a) Combining techniques

|     File                |                           Description                                                 |
| ----------------------- | ------------------------------------------------------------------------------------- |
|  ccsynch.c              |  A blocking Fetch&Multiply object based on the CC-Synch algorithm [1].                |
|  dsmsynch.c             |  A blocking Fetch&Multiply object based on the DSM-Synch algorithm [1].               |
|  hsynch.c               |  A blocking Fetch&Multiply object based on the H-Synch algorithm [1].                 |
|  sim.c                  |  A wait-free Fetch&Multiply object based on the PSim algorithm [2].                   |
|  osci.c                 |  A blocking Fetch&Multiply object based on the OSCI algorithm [3].                    |
|  oyama.c                |  A blocking Fetch&Multiply object based on the Oyama's algorithm [4].                 |


### b) Concurrent queues

|     File                |                               Description                                             |
| ----------------------- | ------------------------------------------------------------------------------------- |
|  ccqueue.c              |  A blocking concurrent queue implementation based on the CC-Synch algorithm [1].      |
|  dsmqueue.c             |  A blocking concurrent queue implementation based on the DSM-Synch algorithm [1].     |
|  hqueue.c               |  A blocking concurrent queue implementation based on the H-Synch algorithm [1].       |
|  simqueue.c             |  A wait-free concurrent queue implementation based on the SimQueue algorithm [2].     |
|  osciqueue.c            |  A blocking concurrent queue implementation based on the OSCI algorithm [3].          |
|  clhqueue.c             |  A blocking concurrent queue implementation based on CLH locks [5], [6].              |
|  msqueue.c              |  A lock-free concurrent queue implementation based on the algorithm of [7].           |


### c) Concurrent stacks

|     File                |                                  Description                                          |
| ----------------------- | ------------------------------------------------------------------------------------- |
|   ccstack.c             |  A blocking concurrent stack implementation based on the CC-Synch algorithm [1].      |
|   dsmstack.c            |  A blocking concurrent stack implementation based on the DSM-Synch algorithm [1].     |
|   hstack.c              |  A blocking concurrent stack implementation based on the H-Synch algorithm [1].       |
|   simstack.c            |  A blocking concurrent stack implementation based on the SimStack algorithm [2].      |
|   oscistack.c           |  A blocking concurrent stack implementation based on the OSCI algorithm [3].          |
|   lfstack.c             |  A lock-free concurrent stack implementation based on the algorithm presented in [8]. |
|   clhstack.c            |  A blocking concurrent stack implementation based on the CLH locks [5, 6].            |


### d) Locks

|     File                |                      Description                                                      |
| ----------------------- | ------------------------------------------------------------------------------------- |
|  clh.c                  |  A blocking Fetch&Multiply object based on the CLH locks [5, 6].                      |
|  mcs.c                  |  A blocking Fetch&Multiply object based on the MCS locks [9].                         |

### e) Hash-tables

|     File                |                   Description                                                         |
| ----------------------- | ------------------------------------------------------------------------------------- |
|  clhhash.c              |  A hash-table implementation based on CLH locks [5, 6].                               |
|  dsmhash.c              |  A hash-table implementation based on MCS locks [9].                                  |

### f) Other benchmarks

|     File                |                                Description                                            |
| ----------------------- | ------------------------------------------------------------------------------------- |
|  lfuobjectbench.c       | A simple, lock-free Fetch&Multiply object implementation.                             |
|  fadbench.c             | A benchmark that measures the throughput of Fetch&Add instructions.                   |
|  activesetbench.c       | A simple implementation of an active-set.                                             |
|  pthreadsbench.c        | A benchmark for measuring the performance os spin-locks of pthreads library.          |

# Compiling the library

In case that you just want to compile the library that provides all the implemented concurrent algorithms
execute one of the following make commands. This step is not necessary in case that you want to run benchmarks.

|     Command             |                                Description                                            |
| ----------------------- | ------------------------------------------------------------------------------------- |
|  `make`                 |  Auto-detects the current architecture and compiles the library/benchmarks for it.    |
|  `make CC=cc ARCH=arch` |  Compiles the library/benchmarks for the current architecture using the cc compiler.  |
|  `make icc`             |  Compiles the library/benchmarks using the icc compiler on some x86/x86_64 machine.   |
|  `make clean`           |  Cleaning-up all binary files.                                                        |


# References

[1]. Fatourou, Panagiota, and Nikolaos D. Kallimanis. "Revisiting the combining synchronization technique".
    ACM SIGPLAN Notices. Vol. 47. No. 8. ACM, PPoPP 2012.

[2]. Fatourou, Panagiota, and Nikolaos D. Kallimanis. "A highly-efficient wait-free universal construction".
    Proceedings of the twenty-third annual ACM symposium on Parallelism in algorithms and architectures.
    SPAA, 2011.

[3]. Fatourou, Panagiota, and Nikolaos D. Kallimanis. "Lock Oscillation: Boosting the Performance of Concurrent 
    Data Structures". Proceedings of the 21st International Conference on Principles of Distributed Systems.
    Opodis 2017.

[3]. Oyama, Yoshihiro, Kenjiro Taura, and Akinori Yonezawa. "Executing parallel programs with synchronization 
    bottlenecks efficiently". Proceedings of the International Workshop on Parallel and Distributed Computing
    for Symbolic and Irregular Applications. Vol. 16. 1999.

[5]. T. S. Craig. "Building FIFO and priority-queueing spin locks from atomic swap". 
    Technical Report TR 93-02-02, Department of Computer Science, University of Washington, February 1993

[6]. Magnusson, Peter, Anders Landin, and Erik Hagersten. "Queue locks on cache coherent multiprocessors".
    Parallel Processing Symposium, 1994. Proceedings., Eighth International. IEEE, 1994
    
[7]. Michael, Maged M., and Michael L. Scott. "Simple, fast, and practical non-blocking and blocking concurrent
    queue algorithms". Proceedings of the fifteenth annual ACM symposium on Principles of distributed 
    computing. ACM, 1996.
    
[8]. Treiber, R. Kent. "Systems programming: Coping with parallelism".
    International Business Machines Incorporated, Thomas J. Watson Research Center, 1986.

[9]. Mellor-Crummey, John M., and Michael L. Scott. "Algorithms for scalable synchronization on shared-memory 
    multiprocessors". ACM Transactions on Computer Systems (TOCS) 9.1 (1991): 21-65.

# Contact

For any further information, please do not hesitate to
send an email at nkallima (at) ics.forth.gr. Feedback is always valuable.

