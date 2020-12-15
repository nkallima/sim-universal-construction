#include <unistd.h>
#include <getopt.h>

#include <bench_args.h>
#include <primitives.h>
#include <config.h>
#include <hsynch.h>

static void printHelp(const char *exec_name) {
    fprintf(stderr,
            "Usage: %s OPTION1 NUM1  OPTION2 NUM2...\n"
            "The following options are available:\n"
            "-t,  --threads    \t set the number of threads to be used in the benchmark (fiber threads also included)\n"
            "-f,  --fibers     \t set the number of user-level threads per posix thread\n"
            "-n,  --numa_nodes \t set the number of numa nodes (which may differ with the actual hw numa nodes) that hierarchical algorithms should take account\n"
            "-r,  --runs       \t set the number of runs that the benchmarked operation should be executed\n"
            "-w,  --max_work   \t set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the benchmarked object), default is 64\n"
            "-b,  --backoff, --backoff_high \t set an upper backoff bound\n"
            "-l,  --backoff_low\t set a lower backoff bound\n"
            "\n"
            "-h, --help        \t displays this help and exits\n",
            exec_name);
}

void parseArguments(BenchArgs *bench_args, int argc, char *argv[]) {
    int opt, long_index;

    static struct option long_options[] =
            {{"threads", required_argument, 0, 't'},
             {"fibers", required_argument, 0, 'f'},
             {"runs", required_argument, 0, 'r'},
             {"max_work", required_argument, 0, 'w'},
             {"backoff_low", required_argument, 0, 'l'},
             {"backoff_high", required_argument, 0, 'b'},
             {"numa_nodes", required_argument, 0, 'n'},
             {"help", no_argument, 0, 'h'},
             {0, 0, 0, 0}};

    // Setting some default values, the user may overide them
    bench_args->nthreads = getNCores();
    bench_args->runs = RUNS;
    bench_args->fibers_per_thread = _DONT_USE_UTHREADS_;
    bench_args->max_work = MAX_WORK;
    bench_args->backoff_high = 0;
    bench_args->backoff_low = 0;
    bench_args->numa_nodes = HSYNCH_DEFAULT_NUMA_POLICY;

    while ((opt = getopt_long(argc, argv, "t:f:r:w:b:l:n:h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 't':
            bench_args->nthreads = atoi(optarg);
            break;
        case 'f':
            bench_args->fibers_per_thread = atoi(optarg);
            break;
        case 'r':
            bench_args->runs = atol(optarg);
            break;
        case 'w':
            bench_args->max_work = atoi(optarg);
            break;
        case 'b':
            bench_args->backoff_high = atoi(optarg);
            break;
        case 'l':
            bench_args->backoff_low = atoi(optarg);
            break;
        case 'n':
            bench_args->numa_nodes = atoi(optarg);
            break;
        case 'h':
            printHelp(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case ':':
            printHelp(argv[0]);
            exit(EXIT_FAILURE);
            break;
        case '?':
            printHelp(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }
    bench_args->runs /= bench_args->nthreads;

#ifdef DEBUG
    fprintf(stderr,
            "DEBUG: threads: %d -- fibers_per_thread: %d -- runs_per_thread: %ld -- max_work: %d\n",
            bench_args->nthreads,
            bench_args->fibers_per_thread,
            bench_args->runs,
            bench_args->max_work);
#endif
}
