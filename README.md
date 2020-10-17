# Summary

This is a collection of optimized concurrent shared data structures which are based on Sim universal
construction algorithm. Current version of the code is optimized for x86_64 machine architectures. 
Some of the benchmarks run much better in machine architectures that natively support Fetch&Add
instructions(e.g., x86_64, etc.).

However, some of the benchmarks have successfully tested in other machine architectures,
such as SPARC, ARM-V8 and RISC-V.

As a compiler, gcc is supported, but you may also try to use icc or clang.
For compiling, it is highly recommended to use gcc of version 4.3.0 or greater. 
For getting the best performance, changes in Makefile may be needed (compiler flags etc).
Important parameters for the benchmarks and/or library are set in the config.h file.


# Running Benchmarks

For running benchmarks use the bench.sh script file that is provided in the main directory of this source tree.

Example usage: `./bench.sh FILE.c OPTION1=NUM1  OPTION2=NUM2 ...`

The following options are available:

|     Option          |                       Description                                            |
| ------------------- | ---------------------------------------------------------------------------- |
|  `-t`, `--threads`  |  set the number of threads (fiber threads also included, if any) to be used in the benchmark |
|  `-f`, `--fibers`   |  set the number of user-level threads per posix thread |
|  `-c`, `--cores`    |  set the number of cores to be used by the benchmark |
|  `-r`, `--repeat`   |  set the number of times that the benchmark should be executed, default is 10 times |
|  `-w`, `--workload` |  set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the  |benchmarked object), default is 64
|  `-l`, `--list`     |  displays the list of the available benchmarks |
|  `-b`, `--backoff`, `--backoff_high` |  set an upper backoff bound for lock-free and Sim-based algorithms |
|  `-bl`, `--backoff_low`  |  set a lower backoff bound (only for msqueue, lfstack and lfuobject benchmarks) |
|  `-h`, `--help`     |  displays this help and exits |


# Collection

The current version of this library provides the following concurrent object implementations/benchmarks:

### a) Combining techniques

|     File               |                           Description                                   |
| ---------------------- | ----------------------------------------------------------------------- |
|  ccsynchbench.c        |  A blocking Fetch&Multiply object based on the CC-Synch algorithm [1].  |
|  dsmsynchbench.c       |  A blocking Fetch&Multiply object based on the DSM-Synch algorithm [1]. |
|  hsynchbench.c         |  A blocking Fetch&Multiply object based on the H-Synch algorithm [1].   |
|  simbench.c            |  A wait-free Fetch&Multiply object based on the PSim algorithm [2].     |
|  oscibench.c           |  A blocking Fetch&Multiply object based on the OSCI algorithm [3].      |
|  oyamabench.c          |  A blocking Fetch&Multiply object based on the Oyama's algorithm [4].   |


### b) Concurrent queues

|     File               |                               Description                                         |
| ---------------------- | --------------------------------------------------------------------------------- |
|  ccqueuebench.c        |  A blocking concurrent queue implementation based on the CC-Synch algorithm [1].  |
|  dsmqueuebench.c       |  A blocking concurrent queue implementation based on the DSM-Synch algorithm [1]. |
|  hqueuebench.c         |  A blocking concurrent queue implementation based on the H-Synch algorithm [1].   |
|  simqueuebench.c       |  A wait-free concurrent queue implementation based on the SimQueue algorithm [2]. |
|  osciqueue.c           |  A blocking concurrent queue implementation based on the OSCI algorithm [3].      |
|  clhqueuebench.c       |  A blocking concurrent queue implementation based on CLH locks [5], [6].          |
|  msqueuebench.c        |  A lock-free concurrent queue implementation based on the algorithm of [7].       |


### c) Concurrent stacks

|     File               |                                  Description                                          |
| ---------------------- | ------------------------------------------------------------------------------------- |
|   ccstackbench.c       |  A blocking concurrent stack implementation based on the CC-Synch algorithm [1].      |
|   dsmstackbench.c      |  A blocking concurrent stack implementation based on the DSM-Synch algorithm [1].     |
|   hstackbench.c        |  A blocking concurrent stack implementation based on the H-Synch algorithm [1].       |
|   simstackbench.c      |  A blocking concurrent stack implementation based on the SimStack algorithm [2].      |
|   oscistack.c          |  A blocking concurrent stack implementation based on the OSCI algorithm [3].          |
|   lfstackbench.c       |  A lock-free concurrent stack implementation based on the algorithm presented in [8]. |
|   clhstackbench.c      |  A blocking concurrent stack implementation based on the CLH locks [5, 6].            |


### d) Locks

|     File              |                      Description                                   |
| --------------------- | ------------------------------------------------------------------ |
|  clhbench.c           |  A blocking Fetch&Multiply object based on the CLH locks [5, 6].   |
|  mcsbench.c           |  A blocking Fetch&Multiply object based on the MCS locks [9].      |

### e) Hash-tables

|     File              |                   Description                              |
| --------------------- | ---------------------------------------------------------- |
|  clhhashbench.c       |  A hash-table implementation based on CLH locks [5, 6].    |
|  dsmhashbench.c       |  A hash-table implementation based on MCS locks [9].       |

### f) Other benchmarks

|     File              |                                Description                                   |
| --------------------- | ---------------------------------------------------------------------------- |
|  lfuobjectbench.c     | A simple, lock-free Fetch&Multiply object implementation.                    |
|  fadbench.c           | A benchmark that measures the throughput of Fetch&Add instructions.          |
|  activesetbench.c     | A simple implementation of an active-set.                                    |
|  pthreadsbench.c      | A benchmark for measuring the performance os spin-locks of pthreads library. |

# Compiling the library

In case that you just want to compile the library that provides all the implemented concurrent algorithms
execute one of the following make commands. This step is not necessary in case that you want to run benchmarks.

|     Command             |                                Description                                           |
| ----------------------- | ------------------------------------------------------------------------------------ |
|  `make`                 |  Auto-detects the current architecture and compiles the library/benchmarks for it.   |
|  `make CC=cc ARCH=arch` |  Compiles the library/benchmarks for the current architecture using the cc compiler  |
|  `make icc`             |  Compiles the library/benchmarks using the icc compiler on some x86/x86_64 machine.  |
|  `make clean`           |  Cleaning-up all binary files.                                                       |


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
