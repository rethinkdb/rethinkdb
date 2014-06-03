#!/bin/bash

set -e
set -o nounset
set -o pipefail

function on_err() {
    echo "ERROR $0:$1" >&2
    echo "Files in $tmpdir." >&2
}
trap 'on_err $LINENO' ERR

function err() {
    figlet "ERROR" >&2
    echo "$@" >&2
    echo "Files in $tmpdir." >&2
    exit 1
}
function highlight() {
    printf "%80s\n" '*' | tr ' ' '*'
    echo "$@"
    printf "%80s\n" '*' | tr ' ' '*'
}
function quickwait() {
    start=`date +%s`
    wait "$@"
    stop=`date +%s`
    if [[ $((stop-start)) -ge 5 ]]; then
        err "Quickwait took more than 5 seconds."
    fi
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
{
    admin create table t3 --database test
    admin pin shard test.t3 -inf-+inf --master 1
} &
t3=$!

wait $t2 || err "Admin failure."
TIMEOUT=15 $tstdir/wait_for_table.rb 1 t2 >>wait2.t
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

TIMEOUT=15 $tstdir/wait_for_table.rb 1 t2 >>wait2.t
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
TIMEOUT=15 $tstdir/wait_for_table.rb 1 t >>wait.t
################################################################################
##### Test that killing a replica does not interrupt feeds.
################################################################################
figlet "Replica"
TIMEOUT=10 $tstdir/cfeed_src.rb 1 t &
src_pid=$!
TIMEOUT=10 $tstdir/cfeed_sink.rb 1 t 't_replica.sink' &
sink_pid=$!
sleep 3
kill $server_3
kill -9 $server_4
quickwait $server_3
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
kill $server_2
quickwait $server_2
set +e
wait $sink_pid && err "Sink succeeded after killing master."
wait $src_pid && err "Src succeeded after killing master."
set -e
highlight `wc t_master.sink`

wait $t3
TIMEOUT=15 $tstdir/wait_for_table.rb 1 t3 >>wait3.t
################################################################################
##### Can shut down the client host reasonably quickly.
################################################################################
figlet "Host"
TIMEOUT=10 $tstdir/cfeed_src.rb 1 t3 &
src_pid=$!
TIMEOUT=10 $tstdir/cfeed_sink.rb 1 t3 't_host.sink' &
sink_pid=$!
sleep 3
kill $server_1
quickwait $server_1
set +e
wait $sink_pid && err "Sink succeeded after killing query host."
wait $src_pid && err "Src succeeded after killing query host."
set -e
highlight `wc t_host.sink`