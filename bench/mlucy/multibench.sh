#!/bin/bash
. `cd -P -- "$(dirname $0)" && pwd -P`/init.sh
num=$1
bench=`cd -P -- "$(dirname $2)" && pwd -P`/"$(basename $2)"
parallel -uj0 "mkdir -p {}; cd {}; bench.sh $bench '-{}' >STDOUT.t" ::: `seq $num`
