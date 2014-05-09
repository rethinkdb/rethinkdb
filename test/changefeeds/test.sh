#!/bin/bash

set -e
set -o nounset
set -o pipefail

HOSTNAME=`hostname`
tag() {
    echo 1
}

QUIET=
rm -f *log.t sink_*.t
deeplog() {
    tag="$HOSTNAME@`date +%s.%N`"
    space=`printf "%-$1s" ""`
    shift
    { [[ -z "$@" ]] && stdbuf -o0 sed "s/^/$tag$space /" || echo "$tag$space $@"; } \
        | tee -a log.t \
        | { [[ -z $QUIET ]] && cat 1>&2 || cat 1>/dev/null; }
}
log() {
    deeplog 0 "$@"
}

abort() {
    echo "ABORTING ($@)"
    log "ABORTING ($@)"
    exit 1
}

# servers="arclight sinister"
servers="arclight sinister"
nodes_per=2
joinline=
for server in $servers; do
    for n in `seq $nodes_per`; do
        joinline+=" --join $server:$((29015+n))"
    done
done
echo "Nodes:$joinline"

id=~/.ssh/test_rsa
[[ ! -e $id ]] && abort "No file $id."
eval "`ssh-agent | head -2` 2>&1 | QUIET=1 log"
ssh-add $id 2>&1 | QUIET=1 log
run() {
    server=$1
    n=$2
    shift 2
    ssh -T -oBatchMode=yes test@$server <<EOF
mkdir -p /mnt/ssd/test/$n
cd /mnt/ssd/test/$n
`cat`
EOF
}

log "Setting up servers..."
for server in $servers; do
    for n in `seq $nodes_per`; do
        run $server $n <<EOF 2>&1 | deeplog 2 &
mkdir -p /mnt/ssd/test/$n
swapoff -a
EOF
        rsync -avz \
            ~/rethinkdb/build/debug/rethinkdb \
            ~/rethinkdb/build/debug/rethinkdb_web_assets \
            test@$server:/mnt/ssd/test/$n 2>&1 | QUIET=1 deeplog 2 &
    done
done
wait

log "Cleansing servers..."
for server in $servers; do
    for n in `seq $nodes_per`; do
        run $server $n <<EOF 2>&1 | HOSTNAME=$server.$n QUIET=1 deeplog 2 &
killall -9 rethinkdb
mv rethinkdb_data rethinkdb_data_old
rm -r rethinkdb_data_old &
EOF
    done
done
wait

log "Launching servers..."
for server in $servers; do
    for n in `seq $nodes_per`; do
        run $server $n <<EOF 2>&1 | HOSTNAME=$server.$n QUIET=1 deeplog 2 &
echo ./rethinkdb -o $n --bind all $joinline
./rethinkdb -d rethinkdb_data -o $n --bind all $joinline 2>&1 | tee -a server_log.t
wait
EOF
    done
done

log "Initializing servers..."
ruby ~/rethinkdb/test/changefeeds/setup.rb arclight 1 2>&1 | QUIET=1 log

(
    shards="S2 S4 S6 S8 Sa Sc Se"
    log "Configuring cluster..."
    ~/rethinkdb/build/debug/rethinkdb admin --join arclight:29016 \
        split shard test.test $shards 2>&1 | QUIET=1 log &
    ~/rethinkdb/build/debug/rethinkdb admin --join arclight:29016 \
        set replicas test.test 2 2>&1 | QUIET=1 log &
    wait
)

log "Waiting for table..."
ruby ~/rethinkdb/test/changefeeds/wait_for_table.rb arclight 1 2>&1 | QUIET=1 log

(
    log "Spawning changefeed sources..."
    for server in $servers; do
        for n in `seq $nodes_per`; do
            ruby ~/rethinkdb/test/changefeeds/cfeed_src.rb $server $n 2>&1 \
                | tee -a srclog.t | log &
        done
    done

    log "Spawning changefeed sinks..."
    for server in $servers; do
        for n in `seq $nodes_per`; do
            ruby ~/rethinkdb/test/changefeeds/cfeed_sink.rb $server $n 2>&1 \
                | tee -a sinklog.t | log &
        done
    done
    wait
)

log "DONE"