#!/bin/bash
set -e
set -o nounset
function on_err() {
    echo "ERROR $0:$1" >&2
    cat setup.log
}
trap 'on_err $LINENO' ERR

poc=$1
poc_offset=$2

echo "Configuring table..." >&2

set +e
trap '' ERR
false
while [[ $? != 0 ]]; do
    `dirname $0`/rethinkdb admin --join $poc:$((29015+$poc_offset)) \
        >>setup.log 2>>setup.log <<EOF
create database test
create table bench --database test
split shard bench  'S0800' 'S1000' 'S1800' 'S2000' 'S2800' 'S3000' 'S3800' 'S4000' 'S4800' 'S5000' 'S5800' 'S6000' 'S6800' 'S7000' 'S7800' 'S8000' 'S8800' 'S9000' 'S9800' 'Sa000' 'Sa800' 'Sb000' 'Sb800' 'Sc000' 'Sc800' 'Sd000' 'Sd800' 'Se000' 'Se800' 'Sf000' 'Sf800'
set durability bench --soft
EOF
    sleep 1
done
trap 'on_err $LINENO' ERR
set -e

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
linger{r.connect(:host => '$poc', :port => 28015+$poc_offset).repl}
linger{r.table('bench').run}
EOF