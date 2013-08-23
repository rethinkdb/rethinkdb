#!/bin/bash
. `cd -P -- "$(dirname $0)" && pwd -P`/init.sh
multibench=`cd -P -- "$1" && pwd -P`
load_conf "$multibench"

echo "$CONFS" | {
    while read line; do
        set -o pipefail
        (
            eval "$line"
            name=`eval echo $NAME`
            mkdir -p $name
            cd $name
            for i in `seq $RUNS`; do
                figlet "$name $i/$RUNS:" >&2
                bench.sh "$multibench/$SUBCONF" >> mbruns.t
            done
            echo "$name: `<mbruns.t confint`"
        ) | tee -a out.t >&2
        set +o pipefail
    done
}
cat out.t
