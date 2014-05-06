#!/bin/bash

set -e
set -o nounset

hostname=`hostname`
tag() {
    echo 1
}

QUIET=
deeplog() {
    tag="$hostname@`date +%s.%N`"
    space=`printf "%-$1s" ""`
    shift
    { [[ -z "$@" ]] && sed "s/^/$tag$space /" || echo "$tag$space $@"; } \
        | tee -a log.t | { [[ -z $QUIET ]] && cat || true; } 1>&2
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
servers="arclight arclight"
nodes_per_server=2
id=~/.ssh/test_rsa
[[ ! -e $id ]] && abort "No file $id."
eval "`ssh-agent | head -2` 2>&1 | QUIET=1 log"
ssh-add $id 2>&1 | QUIET=1 log
each_node() {
    msg=$1
    shift
    log "$msg START"
    for server in $servers; do
        for n in `seq $nodes_per_server`; do
            ssh -T -oBatchMode=yes test@$server <<EOF 2>&1 | deeplog 2 &
export PORT_OFFSET=$n
mkdir -p /mnt/ssd/test/$n
cd /mnt/ssd/test/$n
$@
EOF
        done
    done
    wait
    log "$msg DONE"
}

log "syncing..."
for server in $servers; do
    for n in `seq $nodes_per_server`; do
        rsync -avz \
            ~/rethinkdb/build/debug/rethinkdb \
            test@$server:/mnt/ssd/test/$n/rethinkdb 2>&1 | QUIET=1 log &
    done
done
wait
log "synced"

each_node "pwd" \
    'pwd; nohup rethinkdb -o $PORT_OFFSET --bind all >log1.t 2>log2.t &'
