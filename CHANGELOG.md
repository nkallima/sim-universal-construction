v2.1.1
------
- Improving the ouput messages of build-system.
- Adding support for clang in Makefile (i.e.,use `make clang` for building the sources with the clang compile).
- A few bug-fixes and minor improvements.

v2.1.0
------
- Siginficant enhancements on bench.sh.
- New folder/file library structure.
- ftime is replaced by clock_gettime.

v2.0.1
------
- Removing dead-code from the unsupported sparc/solaris architecture.
- PAPI and NUMA libraries automatically used depending user's settings in config.h.
- Pid fix for the Sync family of algorithms.

v2.0
----
- Initial support for ARM-V8 and RISC-V machine architectures.
- Significant performance optimizations for the Synch family of algorithms on modern NUMA machines.
- The produced binaries can run with any number of threads/fibers without re-compilation.
- All the produced benchmark binaries can accept arguments, such as the number of threads, the number of benchmark's iterations, etc.
- Enhancements on how the runtime handles the NUMA characteristics of modern multiprocessors.
- Better thread affinity policy in NUMA machines.
- A new script called run_all.sh is provided for automatically testing all the produced binaries.
- A .clang-format file is provided in order to maintain the styling-consistency of the source code.
- Numerous performance optimizations and bug-fixes.

v1.9.1
------
- Testing has been performed in RISC-V and aarch64 machine architectures.
- Bug fixes and performance improvements.

v1.9
----
- Support for ARM-V8 and RISC-V machine architectures.

v1.8
----
- The last version that supports solaris/sparc architecture. From now on sparc/solaris architecture is unsupported.
