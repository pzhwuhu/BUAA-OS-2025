#!/bin/bash
#First you can use grep (-n) to find the number of lines of string.
#Then you can use awk to separate the answer.
source=$1
pattern=$2
output=$3
grep -n "$pattern" $source | awk -F ':' '{print $1}' > $output
