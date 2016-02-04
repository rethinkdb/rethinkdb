#!/usr/bin/env ruby
# Copyright 2015 RethinkDB, all rights reserved.
require 'eventmachine'

require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

dbName = 'test'
tableName = File.basename(__FILE__).gsub('.', '_')

r.expr([dbName]).set_difference(r.db_list()).for_each{|row| r.db_create(row)}.run
r.expr([tableName]).set_difference(r.db(dbName).table_list()).for_each{|row| r.db(dbName).table_create(row)}.run
$t = r.db(dbName).table(tableName)
$t.index_create('a').run rescue nil
$t.index_wait('a').run
$t.delete.run

$ticks = 0
# Don't change $n or $lim without thinking about it.  They were picked
# experimentally to produce a good mix of operations that operate on
# the boundary of the set as well as its middle.
$n = 6
$lim = 10
$log = []
$state = []
$stopping = false
$hist = {}
def tick
  if ($ticks += 1) >= 100
    if !$stopping
      $stopping = true
      EM.defer {
        $c.noreply_wait
        # It sucks that we have to sleep for this long, but otherwise
        # the test sometimes fails when running against gdb.  We can't
        # use the normal trick where we do enough inserts that they're
        # guaranteed to happen on every hash shard and then wait for
        # them, because the orderby limit changefeed might not see
        # them all.  We can stop being stupid once we have write
        # timestamps.
        sleep 10
        $h.stop
        EM.stop
      }
    end
  else
    $t.insert({a: rand($n)}).run(noreply: true)
    $t.get_all(rand($n), index: 'a').update({b: rand}).run(noreply: true)
    $t.get_all(rand($n), index: 'a').update({a: rand($n)}).run(noreply: true)
    $t.get_all(rand($n), index: 'a').delete.run(noreply: true)
  end
end

def assert_eq(a, b)
  raise RuntimeError, "Error: #{a} != #{b}" if a != b
end

class H < RethinkDB::Handler
  def on_val(v)
    $log << v
    if v['old_val']
      assert_eq($state[v['old_offset']], v['old_val'])
      $state.delete_at(v['old_offset'])
    end
    if v['new_val']
      $state.insert(v['new_offset'], v['new_val'])
    end
    $hist[$state.size] = ($hist[$state.size] || 0) + 1
    tick
  end
  def on_open
    tick
  end
end
$h = H.new

EM.run {
  opts = {include_initial: true, include_offsets: true}
  $t.insert([{a: 0}, {a: 1}]).em_run {
    $t.order_by(index: 'a').limit($lim).changes(opts).em_run($h)
  }
}

$expected = $t.order_by(index: 'a').limit($lim).run.to_a
PP.pp $state
PP.pp $expected
PP.pp $hist
assert_eq($state, $expected)
