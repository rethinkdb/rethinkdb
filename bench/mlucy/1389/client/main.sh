#!/bin/bash
set -e
set -o nounset

run_at=$1
nodes=$2

cd `dirname "$0"`
while [[ `date +%s` -lt $run_at ]]; do
    sleep 0.1
done
echo "$run_at $nodes"
