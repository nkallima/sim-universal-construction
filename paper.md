---
title: 'Synch: A framework for concurrent data-structures and benchmarks'
tags:
  - Concurrent data-structures
  - benchmarks
  - queues
  - stacks
  - combining-objects
  - hash-tables
  - locks
  - wait-free
  - lock-free
  - performance evaluation
authors:
  - name: Nikolaos D. Kallimanis
    orcid: 0000-0002-0331-1475
    affiliation: 1
affiliations:
  - name: Institute of Computer Science - Foundation for Research and Technology-Hellas (FORTH-ICS)
    index: 1
date: 8 March 2021
bibliography: paper.bib
---

# Summary

The recent advancements in multicore machines highlight the need to simplify concurrent programming in order to leverage their computational power. One way to achieve this is by designing efficient concurrent data structures (e.g., stacks, queues, hash-tables) and synchronization techniques (e.g., locks, combining techniques) that perform well in machines with large numbers of cores. In contrast to ordinary, sequential data-structures, concurrent data-structures can be accessed and/or modifed by multiple threads simultaneously.

Synch is an open-source framework that not only provides some common high-performant concurrent data-structures, but it also provides researchers with the tools for designing and benchmarking high performant concurrent data-structures. The Synch framework contains a substantial set of concurrent data-structures such as queues, stacks, combining-objects, hash-tables, and locks, and it provides a user-friendly runtime for developing and benchmarking concurrent data-structures. Among other features, the runtime provides functionality for creating threads (both POSIX and user-level) easily, tools for measuring performance, etc. The Synch environment provides extensive and comprehensive documentation for all the implemented concurrent data-structures and developers will find a comprehensive set of tests to ensure quality and reproducibility of the results. Moreover, the provided concurrent data-structures and the runtime are highly optimized for contemporary NUMA multiprocessors, such as AMD Epyc and Intel Xeon.


## Statement of need


The Synch framework aims to provide researchers with the appropriate tools for implementing and evaluating state-of-the-art concurrent objects and synchronization mechanisms. Moreover, the Synch framework provides a substantial set of concurrent data-structures giving researchers/developers the ability not only to implement their own concurrent data-structures, but to compare with some state-of-the-art data-structures. Synch provides many state-of-the-art concurrent objects that are thoroughly tested targeting x86_64 POSIX systems.

The Synch framework has been extensively used for implementing and evaluating concurrent data-structures and synchronization techniques in papers, such as @FK2011;@FK2012;@AKD12;@FK2014;@FK2017;@FKR2018.

## Provided concurrent data-structures

The current version of the Synch framework provides a large set of high-performant concurrent data-structures, such as combining-objects, concurrent queues and stacks, concurrent hash-tables and locks. The cornerstone of the Synch framework are the combining objects. A combining object is a concurrent object/data-structure that is able to simulate any other concurrent object, e.g. stacks, queues, atomic counters, barriers. The Synch framework provides the PSim wait-free combining object [@FK2011;@FK2014], the blocking combining objects CC-Synch, DSM-Synch and H-Synch [@FK2012], and the blocking combining object based on the technique presented in [@Oyama99]. Moreover, the Synch framework provides the Osci blocking, combining technique [@FK2017] that achieves good performance using user-level threads.

In terms of concurrent queues, the Synch framework provides the SimQueue [@FK2011;@FK2014] wait-free queue implementation that is based on the PSim combining object, and the CC-Queue, DSM-Queue and H-Queue [@FK2012] blocking queue implementations based on the CC-Synch, DSM-Synch and H-Synch combining objects. A blocking queue implementation based on the CLH locks [@C93;@MLH94] and the lock-free implementation presented in @MS96 are also provided. In terms of concurrent stacks, the Synch framework provides the SimStack [@FK2011;@FK2014] wait-free stack implementation that is based on the PSim combining object, and the CC-Stack, DSM-Stack and H-Stack [@FK2012] blocking stack implementations based on the CC-Synch, DSM-Synch and H-Synch combining objects. Moreover, the lock-free stack implementation of @T86 and the blocking implementation based on the CLH locks [@C93;@MLH94] are provided.
The Synch framework also provides concurrent queue and stacks implementations (OsciQueue and OsciStack implementations) that achieve very high performance using user-level threads [@FK2017].

Furthermore, the Synch framework provides a few scalable lock implementations: the MCS queue-lock presented in @MCS91 and the CLH queue-lock presented in @C93;@MLH94. Finally, the Synch framework provides two example-implementations of concurrent hash-tables. More specifically, it provides a simple implementation based on CLH queue-locks [@C93;@MLH94] and an implementation based on the DSM-Synch [@FK2012] combining technique.

The following table presents a summary of the concurrent data-structures offered by the Synch framework.

| Concurrent  Object    |                Provided Implementations                                               |
| --------------------- | ------------------------------------------------------------------------------------- |
| Combining Objects     | CC-Synch, DSM-Synch and H-Synch [@FK2012]                                             |
|                       | PSim [@FK2011;@FK2014]                                                                |
|                       | Osci [@FK2017]                                                                        |
|                       | Oyama [@Oyama99]                                                                      |
| Concurrent Queues     | CC-Queue, DSM-Queue and H-Queue [@FK2012]                                             |
|                       | SimQueue [@FK2011;@FK2014]                                                            |
|                       | OsciQueue [@FK2017]                                                                   |
|                       | CLH-Queue [@C93;@MLH94]                                                               |
|                       | MS-Queue [@MS96]                                                                      |
|                       | LCRQ [@LCRQ;@LCRQurl]                                                                 |
| Concurrent Stacks     | CC-Stack, DSM-Stack and H-Stack [@FK2012]                                             |
|                       | SimStack [@FK2011;@FK2014]                                                            |
|                       | OsciStack [@FK2017]                                                                   |
|                       | CLH-Stack [@C93;@MLH94]                                                               |
|                       | LF-Stack [@T86]                                                                       |
| Locks                 | CLH [@C93;@MLH94]                                                                     |
|                       | MCS [@MCS91]                                                                          |
| Hash Tables           | CLH-Hash [@C93;@MLH94]                                                                |
|                       | A hash-table based on DSM-Synch [@FK2012]                                             |

## Benchmarks and performance optimizations

For almost every concurrent data-structure, Synch provides at least one benchmark for evaluating its performance. The provided benchmarks allow users to assess the performance of concurrent data-structures, as well as to perform some basic correctness tests on them. All the provided benchmarks offer a great variety of command-line options for controlling the duration of the benchmark, the amount of processing cores and/or threads to be used, the contention, the type of threads (user-level or POSIX), etc.


## Source code structure 

The Synch framework (\autoref{fig:code_structure}) consists of three main parts: the Runtime/Primitives, the Concurrent library, and the Benchmarks. The Runtime/Primitives part provides some basic functionality for creating and managing threads, functionality for basic atomic primitives (e.g., Compare\&Swap, Fetch\&Add, fences, simple synchronization barriers), mechanisms for memory allocation/management (e.g., memory pools), functionality for measuring time, reporting CPU counters, etc. Furthermore, the Runtime/Primitives provides a simple and lightweight library of user level-threads [@FK2017] that can be used to evaluate the provided data-structures and algorithms. The Concurrent library utilizes the building blocks of the Runtime/Primitives layer in order to provide all the concurrent data-structures (e.g., combining objects, queues, stacks). For almost every concurrent data-structure or synchronization mechanism, Synch provides at least one benchmark for evaluating its performance.

![Code-structure of the Synch framework.\label{fig:code_structure}](code_structure.png){width=55%}


## Requirements

- A modern 64-bit multi-core machine. Currently, 32-bit architectures are not supported. The current version of this code is optimized for the x86_64 machine architecture, but the code has also been successfully tested on other machine architectures, such as ARM-V8 and RISC-V. Some of the benchmarks perform much better on architectures that natively support Fetch&Add instructions (e.g., x86_64). For the case of x86_64  architecture, the code has been evaluated on numerous Intel and AMD multicore machines. For the case of ARM-V8 architecture, the code has been successfully evaluated on a Trenz Zynq UltraScale+ board (4 A53 Cortex cores) and on a Raspberry Pi 3 board (4 Cortex A53 cores). For the RISC-V architecture, the code has been evaluated on a SiFive HiFive Unleashed (4 U54 RISC‑V cores).
- A recent Linux distribution.
- As a compiler, gcc version 4.8 or greater is recommended, but users may also try icx or clang.
- Building the environment requires the following development packages:
  - `libatomic`
  - `libnuma`
  - `libpapi` in case that the user wants to get some performance statistics for the provided benchmarks.
- For building the documentation (i.e., man-pages), `doxygen` is required.

## Related work

Scal [@scal] is another open-source framework that implements a set of concurrent data-structures. Scal also provides workloads for benchmarking the implemented data-structures and the appropriate infrastructure for developing concurrent data-structures. The provided data-structures types are limited to stacks, queues, dequeues, and pools. The Scal framework does not provide any contemporary combining object, or more sophisticated data-structures, e.g., hash-tables.

In [@FCurl], a few concurrent implementations for stacks and queues are provided. Moreover, a concurrent pairing-heap implementation is provided by [@FCurl]. The [@FCurl] provides a stack, a queue, and a pairing-heap implementations based on the flat-combining synchronization technique [@FC].

The Concurrent Data Structures (CDS) library [@CDSurl] provides several implementations for stacks, queues, hash-tables, and locks. Moreover, the CDS library provides an implementation of flat-combining [@FC], an implementation of a skip-list, and an AVL tree implementation. Although the CDS library provides a rich set of concurrent data-structures, it does not provide any functionality for benchmarking.

The Boost libraries [@Boost] provide a limited set of concurrent data-structures. More specifically, the Boost.Lockfree library provides simple lock-free implementations for a queue, a stack, and a wait-free single-producer/single-consumer queue.

# Acknowledgments

This work was partially supported by the European Commission under the Horizon 2020 Framework Programme for Research and Innovation through the "European Processor Initiative: Specific Grant Agreement 1" (Grant Agreement Nr 826647).

Many thanks to Panagiota Fatourou for all the fruitful discussions and her significant contribution on the concurrent data-structures implementations presented in [@FK2011;@FK2012;@FK2014;@FK2017]. 

Thanks also to Spiros Agathos for his feedback on the paper and committing some valuable patches to the repository. Many thanks also to Eftychia Datsika for her feedback on the paper.

# References

