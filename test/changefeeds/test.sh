#!/bin/bash

set -e
set -o nounset

hostname=`hostname`
tag() {
    echo "$hostname/`date +%s.%N`"
}

log() {
    echo "`tag` $@" | tee -a log.t 1>&2
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
each_node() {
    for server in $servers; do
        for n in `seq $nodes_per_server`; do
            [[ ! -e $id ]] && abort "No file $id."
            ssh -T -oBatchMode=yes -i $id test@$server <<EOF
export PORT_OFFSET=$n
mkdir -p /mnt/ssd/test/$n
cd /mnt/ssd/test/$n
$@
EOF
        done
    done
}

log "Setting up..."
each_node 'pwd; nohup rethinkdb -o $PORT_OFFSET --bind all >log1.t 2>log2.t &'
