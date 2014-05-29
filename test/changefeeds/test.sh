#!/bin/bash

set -e
set -o nounset
set -o pipefail

function on_err() {
    echo "ERROR $0:$1" >&2
    echo "Files in $tmpdir." >&2
}
trap 'on_err $LINENO' ERR

function err() { figlet "ERROR" >&2; echo "$@" >&2; exit 1; }
function highlight() {
    printf "%80s\n" '*' | tr ' ' '*'
    echo "$@"
    printf "%80s\n" '*' | tr ' ' '*'
}

rdbdir=$(cd -P -- `dirname -- $0`/../.. && pwd -P)
tstdir=$rdbdir/test/changefeeds
bindir=$rdbdir/build/debug
tmpdir=`mktemp -d`
pushd $tmpdir

figlet "Servers"
export PORT_OFFSET=${PORT_OFFSET-14000}
export HOST=localhost
for i in {1..4}; do
    mkdir -p $i
    $bindir/rethinkdb -o $((PORT_OFFSET+i)) -n $i --bind all -d $i/rethinkdb_data \
        --join localhost:$((PORT_OFFSET+29016)) &
    eval "server_$i=$!"
done

function admin() {
    $bindir/rethinkdb admin --join localhost:$((PORT_OFFSET+29016)) "$@"
}
admin create database test
{
    admin create table t --database test
    admin set replicas test.t 3
    admin pin shard test.t -inf-+inf --master 2 --replicas 3 4
} &
t1=$!
admin create table t2 --database test &
t2=$!
admin create table t3 --database test &
t3=$!

wait $t2 || err "Admin failure."
TIMEOUT=15 $tstdir/wait_for_table.rb 1 t2 >wait2.t
################################################################################
##### Test that resharding a table interrupts feeds.
################################################################################
figlet 'Reshard'
TIMEOUT=10 $tstdir/cfeed_src.rb 2 t2 &
src_pid=$!
TIMEOUT=10 $tstdir/cfeed_sink.rb 3 t2 't_reshard.sink' &
sink_pid=$!
sleep 3
admin split shard test.t2 S8
set +e
wait $sink_pid && err "Sink succeeded after reshard."
wait $src_pid && err "Src succeeded after reshard replica."
set -e
highlight `wc t_reshard.sink`

TIMEOUT=15 $tstdir/wait_for_table.rb 1 t2 >wait2.t
################################################################################
##### Test that dropping a table interrupts feeds.
################################################################################
figlet 'Drop'
TIMEOUT=10 $tstdir/cfeed_src.rb 3 t2 &
src_pid=$!
TIMEOUT=10 $tstdir/cfeed_sink.rb 4 t2 't_drop.sink' &
sink_pid=$!
sleep 3
admin rm table test.t2
set +e
wait $sink_pid && err "Sink succeeded after drop."
wait $src_pid && err "Src succeeded after drop replica."
set -e
highlight `wc t_drop.sink`

wait $t1 || err "Admin failure."
TIMEOUT=15 $tstdir/wait_for_table.rb 1 t >wait.t
################################################################################
##### Test that killing a replica does not interrupt feeds.
################################################################################
figlet "Replica"
TIMEOUT=10 $tstdir/cfeed_src.rb 1 t &
src_pid=$!
TIMEOUT=10 $tstdir/cfeed_sink.rb 1 t 't_replica.sink' &
sink_pid=$!
kill $server_3
kill -9 $server_4
wait $sink_pid || err "Sink failed after killing replica."
wait $src_pid || err "Src failed after killing replica."
highlight `wc t_replica.sink`

################################################################################
##### Test that killing a master interrupts feeds.
################################################################################
figlet "Master"
TIMEOUT=10 $tstdir/cfeed_src.rb 1 t &
src_pid=$!
TIMEOUT=10 $tstdir/cfeed_sink.rb 1 t 't_master.sink' &
sink_pid=$!
sleep 3
highlight `wc t_master.sink`
kill -9 $server_2
set +e
wait $sink_pid && err "Sink succeeded after killing master."
wait $src_pid && err "Src succeeded after killing master."
set -e
highlight `wc t_master.sink`



# HOSTNAME=`hostname`
# tag() {
#     echo 1
# }

# QUIET=
# rm -f *log.t sink_*.t
# deeplog() {
#     tag="$HOSTNAME@`date +%s.%N`"
#     space=`printf "%-$1s" ""`
#     shift
#     { [[ -z "$@" ]] && stdbuf -o0 sed "s/^/$tag$space /" || echo "$tag$space $@"; } \
#         | tee -a log.t \
#         | { [[ -z $QUIET ]] && cat 1>&2 || cat 1>/dev/null; }
# }
# log() {
#     deeplog 0 "$@"
# }

# abort() {
#     echo "ABORTING ($@)"
#     log "ABORTING ($@)"
#     exit 1
# }

# # servers="arclight sinister"
# servers="arclight sinister"
# nodes_per=2
# joinline=
# for server in $servers; do
#     for n in `seq $nodes_per`; do
#         joinline+=" --join $server:$((29015+n))"
#     done
# done
# echo "Nodes:$joinline"

# id=~/.ssh/test_rsa
# [[ ! -e $id ]] && abort "No file $id."
# eval "`ssh-agent | head -2` 2>&1 | QUIET=1 log"
# ssh-add $id 2>&1 | QUIET=1 log
# run() {
#     server=$1
#     n=$2
#     shift 2
#     ssh -T -oBatchMode=yes test@$server <<EOF
# mkdir -p /mnt/ssd/test/$n
# cd /mnt/ssd/test/$n
# `cat`
# EOF
# }

# log "Setting up servers..."
# for server in $servers; do
#     for n in `seq $nodes_per`; do
#         run $server $n <<EOF 2>&1 | deeplog 2 &
# mkdir -p /mnt/ssd/test/$n
# swapoff -a
# EOF
#         rsync -avz \
#             ~/rethinkdb/build/debug/rethinkdb \
#             ~/rethinkdb/build/debug/rethinkdb_web_assets \
#             test@$server:/mnt/ssd/test/$n 2>&1 | QUIET=1 deeplog 2 &
#     done
# done
# wait

# log "Cleansing servers..."
# for server in $servers; do
#     for n in `seq $nodes_per`; do
#         run $server $n <<EOF 2>&1 | HOSTNAME=$server.$n QUIET=1 deeplog 2 &
# killall -9 rethinkdb
# mv rethinkdb_data rethinkdb_data_old
# rm -r rethinkdb_data_old &
# EOF
#     done
# done
# wait

# log "Launching servers..."
# for server in $servers; do
#     for n in `seq $nodes_per`; do
#         run $server $n <<EOF 2>&1 | HOSTNAME=$server.$n QUIET=1 deeplog 2 &
# echo ./rethinkdb -o $n --bind all $joinline
# ./rethinkdb -d rethinkdb_data -o $n --bind all $joinline 2>&1 | tee -a server_log.t
# wait
# EOF
#     done
# done

# log "Initializing servers..."
# ruby ~/rethinkdb/test/changefeeds/setup.rb arclight 1 2>&1 | QUIET=1 log

# (
#     shards="S2 S4 S6 S8 Sa Sc Se"
#     log "Configuring cluster..."
#     ~/rethinkdb/build/debug/rethinkdb admin --join arclight:29016 \
#         split shard test.test $shards 2>&1 | QUIET=1 log &
#     ~/rethinkdb/build/debug/rethinkdb admin --join arclight:29016 \
#         set replicas test.test 2 2>&1 | QUIET=1 log &
#     wait
# )

# log "Waiting for table..."
# ruby ~/rethinkdb/test/changefeeds/wait_for_table.rb arclight 1 2>&1 | QUIET=1 log

# (
#     log "Spawning changefeed sources..."
#     for server in $servers; do
#         for n in `seq $nodes_per`; do
#             ruby ~/rethinkdb/test/changefeeds/cfeed_src.rb $server $n 2>&1 \
#                 | tee -a srclog.t | log &
#         done
#     done

#     log "Spawning changefeed sinks..."
#     for server in $servers; do
#         for n in `seq $nodes_per`; do
#             ruby ~/rethinkdb/test/changefeeds/cfeed_sink.rb $server $n 2>&1 \
#                 | tee -a sinklog.t | log &
#         done
#     done
#     wait
# )

# log "DONE"