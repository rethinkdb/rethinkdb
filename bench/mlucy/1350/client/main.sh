#!/bin/bash
set -e
set -o nounset

run_at=$1
nodes=$2
hostflags=`for i in $nodes; do echo -n " --host=$i"; done`

cd `dirname "$0"`
while [[ `date +%s` -lt $run_at ]]; do
    sleep 0.1
done
./stress.py \
    --timeout 120 \
    --clients=400 \
    --batch-size=100 \
    --value-size=64 \
    --workload=1/0/3/0/0/0 \
    --table test.bench $hostflags
