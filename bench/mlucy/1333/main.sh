#!/bin/bash
set -e
set -o nounset

run_at=$1
table=`head -1 "$2"`

while [[ `date +%s` -lt $run_at ]]; do
    sleep 0.01
done
echo "Running on `hostname` at `date +%s`..." >&2
/usr/bin/ruby1.9.3 `dirname "$0"`/client.rb "`cat $3`"
