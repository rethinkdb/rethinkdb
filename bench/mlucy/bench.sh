#!/bin/bash
. `cd -P -- "$(dirname $0)" && pwd -P`/init.sh
load_conf "$1"

function on_err() {
    figlet "ERROR" >&2
}
trap on_err ERR

# Used by `admin` command.
export RETHINKDB_FOR_ADMIN=$RETHINKDB_DIR/rethinkdb

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
wait

################################################################################
##### Set up RethinkDB cluster.
################################################################################
# init_clients "`cat client_hosts`" &
init_rdb_cluster "`cat server_hosts`" $SERVER_INSTANCES $RETHINKDB_DIR $STAGING &
tunnel_to_poc $(head -1 server_hosts) 1 >cluster_port
export ADMIN_CLUSTER_PORT=`cat cluster_port`
wait
for i in `seq $TABLES`; do cat /proc/sys/kernel/random/uuid | tr - _; done >tables
parallel -uj0 create_table {} $TABLE_SHARDS "'$TABLE_CONF'" :::: tables

################################################################################
##### Run Benchmark.
################################################################################


# clients=":::: client_hosts ::: `seq $CLIENT_INSTANCES`"


wait
