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
    local num_servers=`wc -w <<< "$servers"`
    local rand=$(((`echo -n 1; </dev/urandom tr -cd 0-9 | head -c3`-1000)%num_servers))
    local server=`awk '{print $'$((rand+1))'}' <<< "$servers"`
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

function get_table_status() {
        ruby <<EOF
dbs = r.db_list.run
linger{r.db_create('test').run} if !dbs.include?('test')
tbls = r.table_list.run
linger{r.table_create('bench').run} if !tbls.include?('bench')
puts(tbls.include?('bench') ? 'nothing' : 'created')
EOF
}

function shard_table() {
    `dirname $0`/rethinkdb admin --join $poc:$((29015+$poc_offset)) \
        >>setup.log 2>>setup.log <<EOF
split shard bench 'S4000' 'S8000' 'Sc000'
EOF
}

function configure_table() {
    `dirname $0`/rethinkdb admin --join $poc:$((29015+$poc_offset)) \
        >>setup.log 2>>setup.log <<EOF
set replicas bench 4
set acks bench 4
EOF
    ruby <<EOF
linger {
  x = tbl.index_list.run
  tbl.index_create('customer_id').run if !x.include?('customer_id')
  tbl.index_create('datetime').run if !x.include?('datetime')
  if !x.include?('compound')
    tbl.index_create('compound'){|row| [row['customer_id'], row['datetime']]}.run
  end
}
while true
  x = linger{tbl.index_list.run}
  break if x.include?('customer_id') && x.include?('datetime') && x.include?('compound')
  STDERR.puts(PP.pp(x, ""))
end
EOF
}

start_time=1360000000
num_insert=1000000
function populate_table() {
    local start_offset=$1
    local end_offset=$2
    ruby <<EOF
canon_row = {
  id: "eac8b0d0-d136-453c-a543-49f9a9e0ebf3",
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
  arr: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
        35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
        69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85],
  arr2: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
         "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22",
         "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33",
         "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44",
         "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55",
         "56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "66",
         "67", "68", "69", "70", "71", "72", "73", "74", "75", "76", "77",
         "78", "79", "80", "81", "82", "83", "84", "85"],

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
  row[:id] = Digest::MD5.hexdigest(id.to_s)
  row[:datetime] = r.epoch_time($start_time+(id.to_f/60))
  row[:type] = "type#{pareto(5)}"
  row[:customer_id] = "customer#{sprintf("%03d", pareto(1000))}"
  rows << row
  if rows.size >= 100
    tbl.insert(rows, durability:'soft').run(noreply:true)
    rows = []
  end
}
tbl.insert(rows, durability:'soft').run
EOF
}

table_status=`get_table_status || echo error`

case $table_status in
    created)
        shard_table
        echo "Populating table..." >&2
        num_clients=10
        for i in `seq $num_clients`; do
            start=$(((num_insert*(i-1))/num_clients))
            end=$(((num_insert*i)/num_clients))
            populate_table $start $end &
        done
        wait
        echo "Configuring table..." >&2
        configure_table
        ;;
    nothing) echo "Table bench already exists..." >&2 ;;
    *) echo "ERROR: `cat setuperr.log`" >&2; exit 1 ;;
esac

ruby <<EOF
linger {
  endtime = r.epoch_time($start_time+($num_insert.to_f/60))
  tbl.between(endtime, nil, :index => 'datetime').delete.run
}
PP.pp tbl.count.run
EOF