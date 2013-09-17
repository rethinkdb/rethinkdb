#!/bin/bash
set -e
set -o nounset

run_at=$1
servers=$2

alpha=${ALPHA-1.160964}

cd `dirname "$0"`
while [[ `date +%s` -lt $run_at ]]; do
    sleep 0.1
done

function ruby() {
    local counter=${1-0}
    local num_servers=`wc -w <<< "$servers"`
    local server=`awk '{print $'$(((counter%num_servers)+1))'}' <<< "$servers"`
    local host=${server%:*}
    local port=${server#*:}
    echo "Connecting to $host $port..." >>main.log
    ruby1.9.3 2>>main.log <<EOF
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


start_time=1360000000
num_insert=100000
function populate_table() {
    local start_offset=$1
    local end_offset=$2
    local counter=$3
    ruby $counter <<EOF
canon_row = {
  customer_id: "mlucy@rethinkdb.com",
  type: "click",
  datetime: r.now,
  nested: {
    foo: "aaaaaaaaaa", bar: "bbbbbbbbbb",
    nested: {
      foo: "cccccccccc", bar: "dddddddddd",
      nested: {foo: "eeeeeeeeee", bar: "ffffffffff"},
      nested2: {foo: "eeeeeeeeee", bar: "ffffffffff"},
      nested3: {foo: "eeeeeeeeee", bar: "ffffffffff"}
    },
    nested2: {
      foo: "cccccccccc", bar: "dddddddddd",
      nested: {foo: "eeeeeeeeee", bar: "ffffffffff"},
      nested2: {foo: "eeeeeeeeee", bar: "ffffffffff"},
      nested3: {foo: "eeeeeeeeee", bar: "ffffffffff"}
    },
    nested3: {
      foo: "cccccccccc", bar: "dddddddddd",
      nested: {foo: "eeeeeeeeee", bar: "ffffffffff"},
      nested2: {foo: "eeeeeeeeee", bar: "ffffffffff"},
      nested3: {foo: "eeeeeeeeee", bar: "ffffffffff"}
    }
  },

  arr: [1234560000, 1234560001, 1234560002, 1234560003, 1234560004,
        1234560005, 1234560006, 1234560007, 1234560008, 1234560009,
        1234560010, 1234560011, 1234560012, 1234560013, 1234560014,
        1234560015, 1234560016, 1234560017, 1234560018, 1234560019,
        1234560020, 1234560021, 1234560022, 1234560023, 1234560024],

  arr2: ["1234560000", "1234560001", "1234560002", "1234560003",
         "1234560004", "1234560005", "1234560006", "1234560007",
         "1234560008", "1234560009", "1234560010", "1234560011",
         "1234560012", "1234560013", "1234560014", "1234560015",
         "1234560016", "1234560017", "1234560018", "1234560019",
         "1234560020", "1234560021", "1234560022", "1234560023",
         "1234560024", "1234560025", "1234560026", "1234560027"],

  flat: "0123456789101112131415161718192021222324252627282930313233343" \
  "5363738394041424344454647484950515253545556575859606162636465666768" \
  "6970717273747576777879808182838485868788899091929394959697989901234" \
  "5678910111213141516171819202122232425262728293031323334353637383940" \
  "4142434445464748495051525354555657585960616263646566676869707172737" \
  "4757677787980818283848586878889909192939495969798990123456789101112" \
  "1314151617181920212223242526272829303132333435363738394041424344454"
}

def pareto(n)
  [n-1, ((1 - (1-rand)**($alpha/($alpha-1)))*n).floor].min
end
require 'digest'
rows = []
concur=5
($start_offset...$end_offset).each {|id|
  row = canon_row.dup
  row[:datetime] = r.epoch_time($start_time+(id.to_f/60))
  row[:type] = "type#{pareto(5)}"
  row[:customer_id] = "customer#{sprintf("%03d", pareto(1000))}"
  rows << row
  if rows.size >= 100
    tbl.insert(rows, durability:'soft').run
    rows = []
  end
}
tbl.insert(rows, durability:'soft').run
EOF
}

echo "Populating table..." >&2
num_clients=16
for i in `seq $num_clients`; do
    start=$(((num_insert*(i-1))/num_clients))
    end=$(((num_insert*i)/num_clients))
    populate_table $start $end $i &
    sleep 0.1
done

