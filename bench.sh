#!/bin/bash

function usage()
{
    echo -e "Usage: ./bench.sh FILE.run OPTION=NUM ...";
    echo -e "This script runs the algorithm of FILE.run multiple times for various number of POSIX threads and calculates the average throughput (operations/second)";
    echo -e ""
    echo -e "The following options are available."
    echo -e "-t, --max_threads \t set the maximum number number of POSIX threads to be used in the last set of iterations of the benchmark, default is the number of system_cpus"
    echo -e "-s, --step \t set the step (extra number of POSIX threads to be used) in succesive set of iterations of the benchmark, default is number of system_cpu/8 or 1"
    echo -e "-f, --fibers  \t set the number of fibers (user-level threads) per posix thread."
    echo -e "-i, --iterations set the number of times that the benchmark should be executed, default is 10"
    echo -e "-r, --runs    \t set the total number of operations executed by the benchmark, default is 1000000"
    echo -e "-w, --max_work\t set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the benchmarked object), default is 64"
    echo -e "-l, --list    \t displays the list of the available benchmarks"
    echo -e "-b, --backoff, --backoff_high \t set a backoff upper bound for lock-free and Sim-based algorithms"
    echo -e "-bl, --backoff_low            \t set a backoff lower bound (only for msqueue, lfstack and lfuobject benchmarks)"
    echo -e ""
    echo -e "-h, --help    \t displays this help and exits"
    echo -e ""
}

STEP_SELETCTED=0
STEP_THREADS=1
MAX_PTHREADS=$(nproc)
FIBERS=""
BACKOFF=""
MIN_BACKOFF=""
ITERATIONS=10
RUNS=""
LIST=0
WORKLOAD=""
NUMA_NODES=""

if [ "$#" = "0" ] || [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    usage;
    exit;
fi

while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    VALUE=`echo $1 | awk -F= '{print $2}'`
    case $PARAM in
        -h | --help)
            usage;
            exit;
            ;;
        -s | --step_threads)
            STEP_THREADS=$VALUE
            STEP_SELETCTED=1
            ;;
        -t | --max_threads)
            MAX_PTHREADS=$VALUE
            ;;
        -f | --fibers)
            FIBERS="-f $VALUE"
            ;;
        -b | --backoff | --backoff_high)
            BACKOFF="-b $VALUE"
            ;;
        -bl | --backoff_low)
            MIN_BACKOFF="-l $VALUE"
            ;;
        -i | --iterations)
            ITERATIONS=$VALUE
            ;;
        -w | --max_work)
            WORKLOAD="-w $VALUE"
            ;;
        -n | --numa_nodes)
            NUMA_NODES="-n $VALUE"
            ;;
        -r | --runs)
            RUNS="-r $VALUE"
            ;;
        -l | --list)
            LIST=1
            ;;
        -*)
            echo "ERROR: unknown parameter \"$PARAM\""
            usage
            exit 1
            ;;
	*)
	    FILE=$PARAM
	    ;;
    esac
    shift
done

if [ $LIST = "1" ]; then
   cd build/bin;
   ls -lafr *.run
   exit -1;
fi

if [ ! -e build/bin/$FILE ]; then
   echo -e "\n" $FILE "is not available for benchmarking.\n"
   echo -e "Available files for benchmarking: "
   cd build/bin;
   ls -lafr *.run
   exit -1;
fi

set -e 

# Calculate step according to user preferences
if [ $STEP_SELETCTED -eq 0 ]; then
    if [ $MAX_PTHREADS -gt 8 ]; then 
        STEP_THREADS=$(($MAX_PTHREADS / 8))
    fi
fi

# Fill the array with the selected number of threads
PTHREADS_ARRAY=()

# Add set of iterations for 1 thread, if needed
if [ $STEP_THREADS -ne 1 ]; then
    PTHREADS_ARRAY+=(1)
fi
# Add intermediate sets
for (( PTHREADS=$STEP_THREADS; PTHREADS<$MAX_PTHREADS; PTHREADS+=$STEP_THREADS ));do
    PTHREADS_ARRAY+=($PTHREADS)
done
# Add set of iterations for max threads
PTHREADS_ARRAY+=($MAX_PTHREADS)

# Make sure that we start from a clean state
rm -rf res.txt

echo -e "\e[33mBenchmark                       :$FILE"
echo -e "Maximum number of POSIX threads :$MAX_PTHREADS"
echo -e "Number of iterations            :$ITERATIONS"
echo -e "Step                            :$STEP_THREADS\e[39m"
echo ""

# Run the selected number of threads
for PTHREADS in "${PTHREADS_ARRAY[@]}"; do
    echo -e "\e[36m$PTHREADS threads\e[39m"
    
    # Redirect stdout to res.txt, stderr to /dev/null
    for (( i=1; i<=$ITERATIONS; i++ ));do
        ./build/bin/$FILE -t $PTHREADS $WORKLOAD $FIBERS $RUNS $NUMA_NODES $BACKOFF $MIN_BACKOFF 1>> res.txt 2> /dev/null;
    done

    awk 'BEGIN {debug_prefix="";
                time = 0;
                throughput = 0;
                failed_cas = 0;
                executed_cas = 0;
                successful_cas = 0;
                executed_swap = 0;
                executed_faa = 0;
                atomics = 0;
                atomics_per_op = 0;
                ops_per_cas = 0;
                i = 0}
                {time += $2;
                throughput += $5;
                debug_prefix = $8;
                failed_cas += $10; 
                executed_cas += $12;
                successful_cas += $14;
                executed_swap += $16;
                executed_faa += $18;
                atomics += $20;
                atomics_per_op += $22;
                ops_per_cas += $24;
                i += 1} 
        END {   time = time/i; throughput = throughput/i;
                printf "average time: %5d msec\t average throughput: %7.2f mops/sec", time, throughput;
                if (debug_prefix == "DEBUG:") {
                    failed_cas = failed_cas/i; print "failed cas: \t", failed_cas, "";
                    executed_cas =executed_cas/i; print "executed cas: \t", executed_cas, "";
                    successful_cas = successful_cas/i; print "successful cas: ", successful_cas, "";
                    executed_swap = executed_swap/i; print "executed swap: \t", executed_swap, "";
                    executed_faa = executed_faa/i; print "executed faa: \t", executed_faa, "";
                    atomics = atomics/i; print "atomics: \t", atomics, "";
                    atomics_per_op = atomics_per_op/i; print "atomics per op: ", atomics_per_op, "";
                    ops_per_cas = ops_per_cas/i; print "operations per cas: ", ops_per_cas, "\n";
                }
            }' res.txt
    echo ""

    # Remove leftovers
    rm -rf res.txt
done
