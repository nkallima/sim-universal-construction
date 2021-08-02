# Contribution

## Source code structure
The Synch framework consists of 3 main parts, i.e. the Runtime/Primitives, the library of concurrent data-structures and the benchmarks  (see the figure below) . The Runtime/Primitives part provides some basic functionality for creating and managing threads, functionality for basic atomic primitives (e.g. Compare&Swap, Fetch&Add, fences, simple synchronization barriers, etc.), mechanisms for basic memory allocation/management (e.g. memory pools, etc.), functionality for measuring time, reporting CPU counters, etc. Furthermore, the Runtime/Primitives provides a simple and lightweight library of user level-threads that can be used in order to evaluate the provided data-structures and algorithms. 

The Concurrent library utilizes the building blocks of the Runtime/Primitives layer in order to provide all the concurrent data-structures (e.g. combining objects, queues, stacks, etc.). For almost every concurrent data-structure or synchronization mechanism, Synch provides at least one benchmark for evaluating its performance.

<p align="center">
    <img src="resources/code_structure.png" alt="The code-structure of the Synch framework" width="50%">
</p>

## First, discuss your issue, change or contribution
Before contributing to this repository, you are encouraged to first discuss the issue, change or contribution that you wish to make. In order to do this, please open an issue or send an email. We are open to discuss new ideas, improvements or any other contributions. Please note that we have a [Code of conduct](https://github.com/nkallima/sim-universal-construction/blob/main/.github/CODE_OF_CONDUCT.md) that should be considered in all your interactions with the project.

## Ready to make a change
- Fork the repository.
- Create a new branch for your patch.
- Make your updates/changes.
- Open a pull request.
- Submit your pull request targeting the current development branch (i.e. the branch with the highest version number).
- Get it reviewed.

## Coding conventions
Before committing a new patch, follow the steps below for checking the coding style of your patch.
- Ensure that you have installed `clang-format` package (version >= 9).
- After cloning the repository you need to run `git config --local core.hooksPath .git_hooks` to enable recommended code style checks (placed in `.clang_format` file) during git commit. If the tool discovers inconsistencies, it will create a patch file. Please follow the instructions to apply the patch before opening a pull request.

In general, please conform your coding styling to the following conventions:
- ident style:
    * Please avoid using tabs, use spaces instead of tabs.
- comments:
    * Comments start with `//`.
    * For exposing documentation to Doxygen start comments with `///`.
- structs:
    * Self-explanatory names: a struct `DoubleEndedQueueStruct` will tell the developer what the struct is used for.
    * Use a capital-letter for the first character of the struct.
    * Please use `typedef` for simplifying struct-naming, e.g. `DoubleEndedQueueStruct` instead of `struct DoubleEndedQueueStruct`.
- functions: for functions the following rules hold:
    * Self-explanatory names: a function `getMemory()` will tell the developer what it returns as well as `getThreadId()`, etc.
    * Avoid using a capital-letter for the first character of the function.
    * All the public functions provided to the end-user should start with the `synch` prefix (starting from v.3.0.0), i.e. `synchGetMemory`.
    * All the internal functions should NOT start with the `synch` prefix, i.e. `getMemory`.
- memory allocation and alignment:
    * For memory management, please do not use `malloc`, `calloc`, etc. functions directly.
    * Please use the memory management functionality provided by `primitives.h`.
    * In case that the functionality of `primitives.h` does not meet the needs of your patch, please consider to expand it.
- atomic primitives and other processor primitives:
    * Do not use `asm` assembly code or compiler intrinsics directly in your code.
    * Please use the primitives provided by `primitives.h`. 
    * In case that the functionality of `primitives.h` does not meet the needs of your patch, please consider to expand it.
- file-naming conventions:
    * The source-code of each concurrent data-structure that is provided is placed under the `libconcurrent/concurrent` directory.
    * Each concurrent data-structure provides a public API that is a `.h` header-file placed under the `libconcurrent/includes` directory.
    * For each concurrent data-structure, at least one benchmark is provided under the `benchmarks` directory.
    * All benchmarks are placed under the benchmarks directory. The source-code of each of them has the `bench.c` suffix, while the prefix is the name of the concurrent data-structure that is benchmarked (i.e. the `benchmarks/ccsynchbench.c` file is a benchmark for the CC-Synch combining object).

## API & Code documentation
- It is strongly recommended to sufficiently comment your code.
- At least, ensure that all the public functionality of your patch provides adequate documentation through Doxygen.

## Basic correctness validation
Whenever the `DEBUG` macro is defined in `libconcurrent/config.h`, most of the provided benchmarks make some basic sanity-checks in order to ensure that the provided concurrent data-structures behave appropriately. For example, in concurrent stack benchmarks where each thread executes a specific amount of Push/Pop pairs of operations on a concurrent stack, the `DEBUG` macro ensures that after the end of the benchmark the stack is empty of elements (given that the amount of elements in the initialization phase of the benchmark is zero). 

The `validate.sh` script compiles the sources in `DEBUG` mode and runs a big set of benchmarks with various numbers of threads. After running each of the benchmarks, the script evaluates the `DEBUG` output and in case of success it prints `PASS`. In case of a failure, the script simply prints `FAIL`.

In case that you want to contribute a new concurrent data-structure, you should provide sanity-check code in a benchmark that should be activated whenever `DEBUG` macro is defined. Please, don't forget to update the `validate.sh` script appropriately.

## Final steps before pulling a request
- Update the README.md with details.
- Log your contribution/changes to the CHANGELOG.md file.
- Increase the version numbers (if needed) in any example-files and the README.md to the new version that this pull Request would represent. The versioning scheme that we use (for releases later than v2.2.0) is [SemVer](https://semver.org/). In short, given a version number MAJOR.MINOR.PATCH, increment the:
    1. MAJOR version when you make incompatible API changes,
    2. MINOR version when you add functionality in a backwards compatible manner, and
    3. PATCH version when you make backwards compatible bug fixes.

