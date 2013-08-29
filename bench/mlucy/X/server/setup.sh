#!/bin/bash
set -e
set -o nounset

host=$1
offset=$2

function ruby() {
    ruby1.9.3 <<EOF
require 'rethinkdb'
include RethinkDB::Shortcuts
def linger
  res = nil
  while !res
    res = yield rescue nil
  end
  res
end
linger{r.connect(:host => '$host', :port => 28015+$offset).repl}
`cat`
EOF
}

table_status=$(
    {
        ruby 2>>setup.log <<EOF
dbs = r.db_list.run
linger{r.db_create('test').run} if !dbs.include?('test')
tbls = r.table_list.run
linger{r.table_create('bench').run} if !tbls.include?('bench')
puts(tbls.include?('bench') ? 'nothing' : 'created')
EOF
    } || echo "error"
)

case $table_status in
    created)
        echo "Configuring table bench..." >&2
        `dirname $0`/rethinkdb admin --join $host:$((29015+$offset)) \
            >>setup.log 2>>setup.log <<EOF
split shard bench 'S4000' 'S8000' 'Sc000'
set replicas bench 2
set acks bench 2
EOF
        ;;
    nothing) echo "Table bench already configured..." >&2 ;;
    *) echo "ERROR: `cat setuperr.log`" >&2 ;;
esac
