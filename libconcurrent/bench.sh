#!/bin/bash

function usage()
{
    echo -e "Usage: ./bench.sh FILE.c OPTION=NUM ...";
    echo -e "This script runs the algorithm of FILE.c 10 times and calculates the average throughput (operations/second)";
    echo -e " "
    echo -e "The following options are available."
    echo -e "-t, --threads \t set the number of threads (fiber threads also included, if any) to be used in the benchmark"
    echo -e "-f, --fibers  \t set the number of user-level threads per posix thread"
    echo -e "-c, --cores   \t set the number of cores to be used by the benchmark"
    echo -e "-r, --repeat  \t set the number of times that the benchmark should be executed, default is 10 times"
    echo -e "-w, --workload\t set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the benchmarked object), default is 64"
    echo -e "-l, --list    \t displays the list of the available benchmarks"
    echo -e "--compiler    \t set the compiler for building the binaries of the benchmark suite, default is the gcc compiler"
    echo -e "-b, --backoff, --lower_backoff \t set a backoff value (only for msqueue, lfstack, simbench, simstack and simqueue benchmarks)"
    echo -e "-b2, --upper_backoff           \t set a backoff value (only for msqueue and lfstack benchmarks)"
    echo -e ""
    echo -e "-h, --help    \t displays this help and exits"
    echo -e ""
}

NCORES=0;
NTHREADS=0;
FIBERS="";
BACKOFF=0;
UPPER_BACKOFF=0;
REPEATS=10;
LIST=0;
WORKLOAD=64;
COMPILER=gcc;

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
        -c | --cores)
            NCORES=$VALUE;
            ;;
        -t | --threads)
            NTHREADS=$VALUE;
            ;;
        -f | --fibers)
            FIBERS=$VALUE;
            ;;
        -b | --backoff | --lower_backoff)
            BACKOFF=$VALUE;
            ;;
        -b2 | --upper_backoff)
            UPPER_BACKOFF=$VALUE;
            ;;
        -r | --repeat)
            REPEATS=$VALUE;
            ;;
        -w | --workload)
            WORKLOAD=$VALUE;
            ;;
        --compiler)
            COMPILER=$VALUE;
            ;;
        -l | --list)
            LIST=1;
            ;;
        -*)
            echo "ERROR: unknown parameter \"$PARAM\""
            usage
            exit 1
            ;;
	*)
	    FILE=$PARAM;
	    ;;
    esac
    shift
done

if [ $LIST = "1" ]; then
   cd benchmarks;
   ls -lafr *.c
   exit -1;
fi

if [ ! -e benchmarks/$FILE ]; then
   echo -e "\n" $FILE "is not available for benchmarking.\n"
   echo -e "Available files for benchmarking: "
   cd benchmarks;
   ls -lafr *.c;
   exit -1;
fi

if [ $NTHREADS = "0" ]; then
    usage;
    exit -1;
fi

if [ $NCORES = "0" ]; then
    NCORES=$(nproc);
fi

set -e 

echo -e "\e[92mConfiguring library... \t\t\t\t\t done"
echo -e "\e[36mNumber of running threads: " $NTHREADS
echo -e "\e[36mNumber of available processing cores: " $NCORES


make clean > built.log && make CC=$COMPILER ARGS="-DN_THREADS=${NTHREADS} -DMAX_WORK=${WORKLOAD}" > built.log

echo -e "\e[92mBuilding Library... \t\t\t\t\t done"
echo -e "\e[36mRunning the benchmark $REPEATS times"
echo -e "\e[39m"

for (( i=1; i<=$REPEATS; i++ ));do
    ./bin/${FILE%.c}.run $BACKOFF $UPPER_BACKOFF >> res.txt;
    tail -1 res.txt;
done;


echo -e "\e[36m"
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
     END {  time = time/i; print "\naverage time: \t", time, "";
            throughput = throughput/i; print "throughput: \t", throughput, "";
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

 echo -e "\e[39m"
