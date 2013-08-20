#!/bin/bash
. `cd -P -- "$(dirname $0)" && pwd -P`/init.sh
bench=$1
load_conf "$bench"

function on_err() {
    figlet "ERROR" >&2
    echo "ERROR $0:$1" >&2
}
trap 'on_err $LINENO' ERR

################################################################################
##### Set up ec2 cluster.
################################################################################
if [[ ! -f server_hosts && ! -f client_hosts ]]; then
    echo "Creating new cluster..." >&2
    new_cluster "$PREFIX-server" $SERVERS > server_hosts &
    new_cluster "$PREFIX-client" $CLIENTS > client_hosts &
else
    cat >&2 <<EOF
Using existing cluster (remove server_hosts or client_hosts to recreate):
  Servers:
`<server_hosts awk '{print "    "$0}'`
  Clients:
`<client_hosts awk '{print "    "$0}'`
EOF
fi

################################################################################
##### Set up RethinkDB cluster.
################################################################################
wait
init_rdb_cluster "`cat server_hosts`" $SERVER_INSTANCES $RETHINKDB_DIR $STAGING &
export POC=`head -1 server_hosts`
tunnel_to_poc $POC 1 >cluster_port
export ADMIN_CLUSTER_PORT=`cat cluster_port`
for i in `seq $TABLES`; do cat /proc/sys/kernel/random/uuid | tr - _; done >tables
wait
parallel -uj0 create_table $POC 1 $STAGING {} $TABLE_SHARDS "'$TABLE_CONF'" :::: tables &

################################################################################
##### Set up clients.
################################################################################
tables=`cat tables`
node_pairs=":::: server_hosts ::: `seq $SERVER_INSTANCES`"
nodes=`parallel -uj0 echo '{1}:$((CLIENT_PORT+{2}))' $node_pairs`
parallel -uj0 dist_bench "$bench" {} $STAGING "'$tables'" "'$nodes'" :::: client_hosts

################################################################################
##### Run Benchmark.
################################################################################
wait
parallel -j0 run_bench {} $STAGING :::: client_hosts

wait
