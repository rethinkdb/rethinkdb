#!/bin/bash
set -e
set -o nounset

run_at=$1
table=`head -1 "$2"`
hostflags=`<"$3" awk '{print " --host="$0}'`

while [[ `date +%s` -le $run_at ]]; do
    sleep 0.001
done
echo "Running on `hostname` at `date +%s`..." >&2

retries=3
while true; do
    set +e
    set -o pipefail
    `dirname "$0"`/stress.py \
        --timeout 120 \
        --clients=32 \
        --batch-size=10 \
        --value-size=10000 \
        --table ${table}_db.$table $hostflags
    ret=$?
    set +o pipefail
    set -e
    if [[ $ret == 0 ]]; then
        exit 0
    elif [[ $retries -le 0 ]]; then
        exit $ret
    else
        retries=$((retries-1))
        sleep 1
    fi
done
