#include <unistd.h>
#include <getopt.h>

#include <bench_args.h>
#include <primitives.h>
#include <config.h>

static void printHelp(void) {
}

void parseArguments(BenchArgs *bench_args, int argc, char *argv[]) {
    int opt, long_index;

    static struct option long_options[] = {
        {"threads",      required_argument, 0,  't' },
        {"fibers",       required_argument, 0,  'f' },
        {"runs",         required_argument, 0,  'r' },
        {"max_work",     required_argument, 0,  'w' },
        {"backoff_low",  required_argument, 0,  'l' },
        {"backoff_high", required_argument, 0,  'h' },
        {0,              0,                 0,   0  }
    };
    
    // Setting some default values, the user may overide them
    bench_args->nthreads = getNCores();
    bench_args->runs = RUNS;
    bench_args->fibers_per_thread = _DONT_USE_UTHREADS_;
    bench_args->max_work = MAX_WORK;
    bench_args->backoff_high = 0;
    bench_args->backoff_low = 0;

    while((opt = getopt_long(argc, argv, "t:f:r:w:h:l:",long_options, &long_index)) != -1) {
        switch(opt) {
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
            case 'h':  
                bench_args->backoff_high = atoi(optarg);  
                break;
            case 'l':  
                bench_args->backoff_low = atoi(optarg);  
                break;   
            case ':':
                printHelp();
                exit(EXIT_FAILURE);
                break;
            case '?':
                printHelp();
                exit(EXIT_FAILURE);
                break;  
        }  
    }
    bench_args->runs /= bench_args->nthreads;

#ifdef DEBUG
    fprintf(stderr, "DEBUG: threads: %d -- fibers_per_thread: %d -- runs_per_thread: %ld -- max_work: %d\n",
            bench_args->nthreads, bench_args->fibers_per_thread, bench_args->runs, bench_args->max_work);
#endif
}
