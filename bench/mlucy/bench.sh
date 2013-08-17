#!/bin/bash
. `cd -P -- "$(dirname $0)" && pwd -P`/init.sh
load_conf "$1"

# Used by `admin` command.
export RETHINKDB_FOR_ADMIN=$RETHINKDB_DIR/rethinkdb

# new_cluster "$PREFIX-server" $SERVERS > server_hosts &
# new_cluster "$PREFIX-client" $CLIENTS > client_hosts &
wait

# clients=":::: client_hosts ::: `seq $CLIENT_INSTANCES`"

init_rdb_cluster "`cat server_hosts`" $SERVER_INSTANCES $RETHINKDB_DIR $STAGING &
tunnel_to_poc $(head -1 server_hosts) 1 >cluster_port
export CLUSTER_PORT=`cat cluster_port`
wait

for i in `seq $TABLES`; do cat /proc/sys/kernel/random/uuid | tr - _; done >tables
parallel -vuj0 create_table {} $TABLE_SHARDS $TABLE_REPLICAS $TABLE_ACKS :::: tables

wait
