#!/bin/bash
set -e
set -o nounset
function on_err() {
    echo "ERROR $0:$1" >&2
}
trap 'on_err $LINENO' ERR

poc=$1
poc_offset=$2
servers=$3
alpha=${ALPHA-1.160964}

function ruby() {
    local counter=${1-0}
    local num_servers=`wc -w <<< "$servers"`
    local server=`awk '{print $'$(((counter%num_servers)+1))'}' <<< "$servers"`
    local host=${server%:*}
    local port=${server#*:}
    echo "Connecting to $host $port..." >>setup.log
    ruby1.9.3 2>>setup.log <<EOF
require 'rethinkdb'
include RethinkDB::Shortcuts
def linger
  res = nil
  while !res
    begin
      res = yield
    rescue RethinkDB::RqlRuntimeError => e
      STDERR.puts(e.class.to_s)
      STDERR.puts(e.to_s)
      STDERR.puts('sleeping...')
      sleep 0.5
    rescue Errno::ECONNREFUSED => e
      STDERR.puts(e.class.to_s)
      STDERR.puts(e.to_s)
      STDERR.puts('sleeping...')
      sleep 0.5
    end
  end
  res
end

linger{r.connect(:host => '$host', :port => $port).repl}
tbl = r.table('bench')
`cat`
EOF
}

function create_table() {
        ruby <<EOF
dbs = r.db_list.run
linger{r.db_create('test').run} if !dbs.include?('test')
tbls = r.table_list.run
linger{r.table_create('bench').run} if !tbls.include?('bench')
EOF
}

create_table
