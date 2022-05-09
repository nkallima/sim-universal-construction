#!/bin/bash

RUNS_PER_THREAD=100000
MAX_PTHREADS=$(nproc)
BIN_PATH="./build/bin"
RES_FILE="res.txt"
BUILD_LOG=$HOME/build.log
STEP_SELETCTED=0
STEP_THREADS=1
WORKLOAD="-w 64"
FIBERS=""
NUMA_NODES=""
CODECOV=0
PASS_STATUS=1

function usage()
{
    echo -e "Usage: ./validate.sh OPTION1 VALUE1 OPTION2 VALUE2 ...";
    echo -e "This script compiles the sources in DEBUG mode and validates the correctnes of some of the provided concurrent objects.";
    echo -e ""
    echo -e "The following options are available."
    echo -e "-t, --max_threads \t set the maximum number of POSIX threads to be used in the last set of iterations of the benchmark, default is the number of system's virtual cores"
    echo -e "-s, --step \t set the step (extra number of POSIX threads to be used) in succesive set of iterations of the benchmark, default is the (number of system/s virtual cores/8) or 1"
    echo -e "-f, --fibers  \t set the number of fibers (user-level threads) per posix thread."
    echo -e "-r, --runs    \t set the total number of operations executed by each thread of each benchmark, default is ${RUNS_PER_THREAD}"
    echo -e "-w, --max_work\t set the amount of workload (i.e. dummy loop iterations among two consecutive operations of the benchmarked object), default is ${WORKLOAD}"
    echo -e "-n, --numa_nodes\t set the number of numa nodes (which may differ with the actual hw numa nodes) that hierarchical algorithms should take account"
    echo -e ""
    echo -e "-h, --help    \t displays this help and exits"
    echo -e ""
}

COLOR_PASS="[ \e[32mPASS\e[39m ]"
COLOR_FAIL="[ \e[31mFAIL\e[39m ]"

declare -a uobjects=("ccsynchbench.run"                     "dsmsynchbench.run" "hsynchbench.run" "oscibench.run"      "simbench.run"      "fcbench.run"      "oyamabench.run" "mcsbench.run" "clhbench.run" "pthreadsbench.run" "fadbench.run")
declare -a queues=(  "ccqueuebench.run" "clhqueuebench.run" "dsmqueuebench.run" "hqueuebench.run" "osciqueuebench.run" "simqueuebench.run" "fcqueuebench.run" "lcrqbench.run")
declare -a stacks=(  "ccstackbench.run" "clhstackbench.run" "dsmstackbench.run" "hstackbench.run" "oscistackbench.run" "simstackbench.run" "fcstackbench.run")

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    usage;
    exit;
fi

while [ "$1" != "" ]; do
    PARAM=`echo $1`
    VALUE=`echo $2`
    SHIFT=0
    case $PARAM in
        -h | --help)
            usage;
            exit;
            ;;
        -s | --step_threads)
            STEP_THREADS=$VALUE
            STEP_SELETCTED=1
            SHIFT=1
            ;;
        -t | --max_threads)
            MAX_PTHREADS=$VALUE
            SHIFT=1
            ;;
        -f | --fibers)
            FIBERS="-f $VALUE"
            SHIFT=1
            ;;
        -w | --max_work)
            WORKLOAD="-w $VALUE"
            SHIFT=1
            ;;
        -n | --numa_nodes)
            NUMA_NODES="-n $VALUE"
            SHIFT=1
            ;;
        -r | --runs)
            RUNS_PER_THREAD=$VALUE
            SHIFT=1
            ;;
        --codecov)
            CODECOV=1
            ;;
        -*)
            echo "ERROR: unknown parameter \"$PARAM\""
            usage
            exit 1
            ;;
    esac
    shift
    if [ $SHIFT = "1" ]; then
        shift
        if [ "$VALUE" = "" ]; then
            echo "ERROR: no value set for \"$PARAM\""
            usage
            exit 1
        fi
    fi
done

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

echo -ne "Compiling the sources...\t\t\t\t\t\t"
if [ $CODECOV -eq 1 ]; then
    make clean codecov &> $BUILD_LOG
else
    make clean debug &> $BUILD_LOG
fi

if [ $? -eq 0 ]; then
    echo -e $COLOR_PASS
else
    echo -e $COLOR_FAIL
    echo -e "\nCheck" $BUILD_LOG "for the build error-log."
    printf "\n\n\e[31mFailed to build validation tests!\n"
    printf "================================\e[39m\n\n"
    exit 1
fi

# Run the selected number of threads
for PTHREADS in "${PTHREADS_ARRAY[@]}"; do
    printf "\n\e[36mValidating for %3d thread(s)\n" $PTHREADS
    echo -e "============================\e[39m"

    runs=$(($RUNS_PER_THREAD * $PTHREADS))

    for bench in "${uobjects[@]}"; do
        printf "Validating %-20s \t\t\t\t\t" $bench
        $BIN_PATH/$bench -t $PTHREADS -r $runs $WORKLOAD $FIBERS $NUMA_NODES > $RES_FILE 2>&1
        # state counts the actual number of the operations applied in the concurrent object
        state=$(fgrep "Object state: " $RES_FILE)
        state=${state/#"DEBUG: Object state: "}
        if [ $state -eq $runs ]; then
            echo -e $COLOR_PASS
        else
            echo -e $COLOR_FAIL
            echo "Expected state: " $runs
            echo "Invalid state: " $state
            PASS_STATUS=0
        fi
    done

    for bench in "${stacks[@]}"; do
        printf "Validating %-20s \t\t\t\t\t" $bench
        $BIN_PATH/$bench -t $PTHREADS -r $runs $WORKLOAD $FIBERS $NUMA_NODES > $RES_FILE 2>&1
        # state counts the actual number of both push and pop operations applied in the concurrent stack
        state=$(fgrep "Object state: " $RES_FILE)
        state=${state/#"DEBUG: Object state: "}
        valid_state=$(($runs * 2))
        if [ $state -eq $valid_state ]
        then
            echo -e $COLOR_PASS
        else
            echo -e $COLOR_FAIL
            echo "Expected state: " $valid_state
            echo "Invalid state: " $state
            PASS_STATUS=0
        fi
    done

    for bench in "${queues[@]}"; do
        printf "Validating %-20s \t\t\t\t\t" $bench
        $BIN_PATH/$bench -t $PTHREADS -r $runs $WORKLOAD $FIBERS $NUMA_NODES > $RES_FILE 2>&1
        # state counts the actual number of the enqueue operations applied in the concurrent queue
        enq_state=$(fgrep "DEBUG: Enqueue: Object state: " $RES_FILE)
        # state counts the actual number of the dequeue operations applied in the concurrent queue
        deq_state=$(fgrep "DEBUG: Dequeue: Object state: " $RES_FILE)
        left_nodes=$(fgrep "nodes were left in the queue" $RES_FILE)
        left_nodes=${left_nodes/#"DEBUG: "}
        left_nodes=${left_nodes/%" nodes were left in the queue"}
        enq_state=${enq_state/#"DEBUG: Enqueue: Object state: "}
        deq_state=${deq_state/#"DEBUG: Dequeue: Object state: "}
        if [ $enq_state -eq $runs ]; then
            if [ $left_nodes -eq 0 ]; then
                echo -e $COLOR_PASS
            else
                echo -e $COLOR_FAIL
                echo "Error state: " $left_nodes "nodes were left in the queue"
                PASS_STATUS=0
            fi
        else
            echo -e $COLOR_FAIL
            echo "Expected state: " $runs
            echo "Invalid state: " $enq_state
            PASS_STATUS=0
        fi
    done
done

rm -f $RES_FILE

if [ $PASS_STATUS -eq 1 ]; then
    printf "\n\n\e[32mAll validation tests passed successfully!\n"
    printf "=========================================\e[39m\n\n"
else
    printf "\n\n\e[31mValidation tests failed!\n"
    printf "========================\e[39m\n\n"
    exit 1
fi
