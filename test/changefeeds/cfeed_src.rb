#!/usr/local/bin/ruby
$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

$port = ENV['PORT_OFFSET'].to_i + 28015 + ARGV[0].to_i
$tbl = ARGV[1]
$start = Time.now
$timeout = ENV['TIMEOUT'].to_i

p "Src..."
r.connect(host: ENV['HOST'], port: $port).repl
docs = (0...1000).map{|i| {n: i, pid: $$, target: "#{ENV['HOST']}:#{$port}"}}
while Time.now < $start + $timeout
  res = r.table($tbl).insert(docs, durability:'soft').run
  r.table($tbl).filter{|row| row['pid'].eq($$) & (row['n'] % 2).eq(1)}.delete.run
  r.table($tbl).filter{|row| row['pid'].eq($$) & (row['n'] % 2).eq(0)}.update {|row|
    { target: row['target'] + " UPDATED", updated: true }
  }.run
end
p "Src DONE."
