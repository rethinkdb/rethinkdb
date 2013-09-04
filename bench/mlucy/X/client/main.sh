#!/bin/bash
set -e
set -o nounset

run_at=$1
table=`head -1 "$2"`
hosts=`cat $3`

while [[ `date +%s` -lt $run_at ]]; do
    sleep 0.01
done
echo "$run_at $table $hosts"
