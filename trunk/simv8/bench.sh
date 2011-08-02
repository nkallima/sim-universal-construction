#!/bin/bash

rm -f a.out res.txt;

echo "Apropriate script use: bench.sh FILE N_THREADS LOWER_BACKOFF UPPER_BACKOFF";
echo "This script runs FILE 10 times using N_THREADS and calulates average exection time";

gcc $1 -O3 -msse3 -ftree-vectorize -ftree-vectorizer-verbose=0 -finline-functions -lpthread -march=native -mtune=native -DN_THREADS=$2 -DUSE_CPUS=$3 -D_GNU_SOURCE -pipe #-I/home/nkallima/papi/include -L/home/nkallima/papi/lib -lpapi

for a in {1..10};do
    ./a.out $4 $5 >> res.txt;
    tail -1 res.txt;
done;

awk 'BEGIN {time = 0;
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
		    failed_cas += $4; 
            executed_cas += $6;
            successful_cas += $8;
            executed_swap += $10;
            executed_faa += $12;
            atomics += $14;
            atomics_per_op += $16;
		    ops_per_cas += $18;
		    i += 1} 
     END {time = time/i;             print "\naverage time: \t", time, "";
	      failed_cas = failed_cas/i; print "failed cas: \t", failed_cas, "";
		  executed_cas =executed_cas/i; print "executed cas: \t", executed_cas, "";
		  successful_cas = successful_cas/i; print "successful cas: ", successful_cas, "";
		  executed_swap = executed_swap/i; print "executed swap: \t", executed_swap, "";
		  executed_faa = executed_faa/i; print "executed faa: \t", executed_faa, "";
		  atomics = atomics/i; print "atomics: \t", atomics, "";
		  atomics_per_op = atomics_per_op/i; print "atomics per op: ", atomics_per_op, "";
		  ops_per_cas = ops_per_cas/i; print "operations per cas: ", ops_per_cas, "\n";
		  }' res.txt
