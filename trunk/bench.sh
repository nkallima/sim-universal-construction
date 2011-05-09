#!/bin/bash

rm -f res.txt;

for a in `seq 1 10`; do
       ./$1 $2 $3 >> res.txt;
       tail -n 1 res.txt;
done

echo Benchmark completed!
awk 'BEGIN {sum = 0; i = 0}
           {sum += $1; i += 1} 
     END {sum = sum/i;print "average completion time: ", sum, "\n"}' res.txt
rm -f res.txt;
