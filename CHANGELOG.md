Changelog
---------

v3.0.0
------
- Header-files cleanup in order to provide a better and consistent API.
- Adding the `Synch` prefix to all structs provided by the runtime/primitives to the end-user.
- Adding the `synch` prefix to all functions provided by the runtime/primitives to the end-user.
- Extensive documentation for the API.
- Create a `doxygen` configuration file dor auto-generating man-pages/documentation.
- API documentation is now provided at GitHub pages: https://nkallima.github.io/sim-universal-construction/index.html.
- A code coverage report is provided through codecov.io for the validation script.
- Support for installing the framework.
- Improvements on the build environment.
- Expansion of the CONTRIBUTING.md.
- Providing a short discussion (see PERFORMANCE.md) of the expected performance for the various objects provided by the framework.

v2.4.0
------
- Adding the LCRQ Queue implementation (Adam Morrison and Yehuda Afek, PPoPP 2013, http://mcg.cs.tau.ac.il/projects/lcrq).
- Memory reclamation for SimStack is supported.
- Adding support for 128 bit Compare&Swap.
- Documentation improvements; a memory reclamation section was added in `README.md`.

v2.3.1
------
- Improvements on `validate.sh` for better error reporting.
- Enhancements on `README.md`.
- Improved logo.

v2.3.0
------
- Introducing `validate.sh`, a smoke/validation script that verifies many of the provided data structures.
- A few improvements on some benchmarks in order to be conformed with `validate.sh`.
- Introducing the logo of the repository.
- Enhancements on `README.md`.

v2.2.1
------
- Executing `Pause` instructions on spinning-loops for the X86 architecture. Spin-locks and some lock-free algorithms may have performance benefits in SMT architectures.  
- Documentation improvements.

v2.2.0
------
- A new implementation of the pool.c functionality. Most stack and queue implementations support memory reclamation, the only exception are the simstack, simqueue, lfstack and msqueue implementations.
- Code clean-up for the Sim family of algorithms.
- Improvements on the HSynch family of algorithms.
- Enhanced NUMA support for the NUMA-aware data-structures.
- Homogenized code for stack and queue implementations.
- Better output for bench.sh script.
- Numerous performance optimizations and bug-fixes, especially in machines with weak memory models.
- Documentation improvements.

v2.1.1
------
- Improving the output messages of build-system.
- Adding support for clang in Makefile (i.e.,use `make clang` for building the sources with the clang compile).
- A few bug-fixes and minor improvements.
- Dropped support for Intel icc compiler and added support for Intel icx compiler. Tested with Intel(R) oneAPI DPC++ Compiler 2021.1.2.

v2.1.0
------
- Significant enhancements on bench.sh.
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
