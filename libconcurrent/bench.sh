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
    echo -e "-b, --backoff \t set a backoff value (only for simbench, simstack and simqueue benchmarks)"
    echo -e "-r, --repeat  \t set the number of times that the benchmark should be executed, default is 10 times"
    echo -e "-l, --list    \t displays the list of the available benchmarks"
    echo -e "--compiler    \t set the compiler for building the binaries of the benchmark suite, default is the gcc compiler"
    echo -e ""
    echo -e "-h, --help    \t display this help and exit;"
    echo -e ""
}

NCORES=0;
NTHREADS=0;
FIBERS="";
BACKOFF=0;
REPEATS=10;
LIST=0;
COMPILER=gcc;

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    usage;
    exit;
fi

if [ $1 = "-l" ] || [ $1 = "--list" ]; then
   cd benchmarks;
   ls -lafr *.c
   exit;
fi

FILE=$1;
shift;

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
        -b | --backoff)
            BACKOFF=$VALUE;
            ;;
        -r | --repeat)
            REPEATS=$VALUE;
            ;;
        --compiler)
            COMPILER=$VALUE;
            ;;
        -l | --list)
            LIST=1;
            ;;
        *)
            echo "ERROR: unknown parameter \"$PARAM\""
            usage
            exit 1
            ;;
    esac
    shift
done

if [ $LIST = "1" ]; then
   ls -lafr ./benchmarks/*.c
   exit;
fi

if [ ! -e benchmarks/$FILE ]; then
   echo -e "\n" $FILE "is not available for benchmarking.\n"
   echo -e " Available files for benchmarking: "
   ls -lafr ./benchmarks/*.c;
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


if [ $COMPILER = "gcc" ]; then
    make clean > built.log && make ARGS="-DN_THREADS=${NTHREADS} -DFIBERS_PER_THREAD=${FIBERS}" ARGCORES="-DUSE_CPUS=${NCORES}" > built.log
else
    make clean > built.log && make $COMPILER ARGS="-DN_THREADS=${NTHREADS} -DFIBERS_PER_THREAD=${FIBERS}" ARGCORES="-DUSE_CPUS=${NCORES}" > built.log
fi

echo -e "\e[92mBuilding Library... \t\t\t\t\t done"
echo -e "\e[36mRunning the benchmark 10 times"
echo -e "\e[39m"

for (( i=1; i<=$REPEATS; i++ ));do
    ./bin/${FILE%.c}.run $BACKOFF >> res.txt;
    tail -1 res.txt;
done;


echo -e "\e[36m"
awk 'BEGIN {time = 0;
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
            throughput += $5
            failed_cas += $9; 
            executed_cas += $11;
            successful_cas += $13;
            executed_swap += $15;
            executed_faa += $17;
            atomics += $19;
            atomics_per_op += $21;
            ops_per_cas += $23;
            i += 1} 
     END {time = time/i;             print "\naverage time: \t", time, "";
            throughput = throughput/i; print "throughput: \t", throughput, "";
            failed_cas = failed_cas/i; print "failed cas: \t", failed_cas, "";
            executed_cas =executed_cas/i; print "executed cas: \t", executed_cas, "";
            successful_cas = successful_cas/i; print "successful cas: ", successful_cas, "";
            executed_swap = executed_swap/i; print "executed swap: \t", executed_swap, "";
            executed_faa = executed_faa/i; print "executed faa: \t", executed_faa, "";
            atomics = atomics/i; print "atomics: \t", atomics, "";
            atomics_per_op = atomics_per_op/i; print "atomics per op: ", atomics_per_op, "";
            ops_per_cas = ops_per_cas/i; print "operations per cas: ", ops_per_cas, "\n";
         }' res.txt

 echo -e "\e[39m"
