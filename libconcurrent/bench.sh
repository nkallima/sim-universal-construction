#!/bin/bash

function usage()
{
    echo -e "Usage: ./bench.sh FILE.run OPTION=NUM ...";
    echo -e "This script runs the algorithm of FILE.run 10 times and calculates the average throughput (operations/second)";
    echo -e " "
    echo -e "The following options are available."
    echo -e "-t, --threads \t set the number of threads (fiber threads also included, if any) to be used in the benchmark"
    echo -e "-f, --fibers  \t set the number of user-level threads per posix thread"
    echo -e "-i, --iterations \t set the number of times that the benchmark should be executed, default is 10 times"
    echo -e "-r, --runs    \t set the number of runs that the benchmarked operation should be executed"
    echo -e "-w, --max_work\t set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the benchmarked object), default is 64"
    echo -e "-l, --list    \t displays the list of the available benchmarks"
    echo -e "-b, --backoff, --backoff_high \t set a backoff upper bound for lock-free and Sim-based algorithms"
    echo -e "-bl, --backoff_low            \t set a backoff lower bound (only for msqueue, lfstack and lfuobject benchmarks)"
    echo -e ""
    echo -e "-h, --help    \t displays this help and exits"
    echo -e ""
}

NTHREADS="";
FIBERS="";
BACKOFF="";
MIN_BACKOFF="";
ITERATIONS=10;
RUNS=""
LIST=0;
WORKLOAD="";
NUMA_NODES="";

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
        -t | --threads)
            NTHREADS="-t $VALUE";
            ;;
        -f | --fibers)
            FIBERS="-f $VALUE";
            ;;
        -b | --backoff | --backoff_high)
            BACKOFF="-b $VALUE";
            ;;
        -bl | --backoff_low)
            MIN_BACKOFF="-l $VALUE";
            ;;
        -i | --iterations)
            ITERATIONS=$VALUE;
            ;;
        -w | --max_work)
            WORKLOAD="-w $VALUE";
            ;;
        -n | --numa_nodes)
            NUMA_NODES="-n $VALUE";
            ;;
        -r | --runs)
            RUNS="-r $VALUE";
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
   cd bin;
   ls -lafr *.run
   exit -1;
fi

if [ ! -e bin/$FILE ]; then
   echo -e "\n" $FILE "is not available for benchmarking.\n"
   echo -e "Available files for benchmarking: "
   cd bin;
   ls -lafr *.run
   exit -1;
fi

set -e 

echo -e "\e[36mNumber of available processing cores: " $(nproc);
echo -e "\e[36mRunning $FILE benchmark $ITERATIONS times"
echo -e "\e[39m"

rm -rf res.txt;

for (( i=1; i<=$ITERATIONS; i++ ));do
    ./bin/$FILE $NTHREADS $WORKLOAD $FIBERS $RUNS $NUMA_NODES $BACKOFF $MIN_BACKOFF >> res.txt;
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
     END {  time = time/i; print "\naverage time: \t\t", time, "";
            throughput = throughput/i; print "average throughput: \t", throughput, "";
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
