require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

dbName = 'test'
tableName = File.basename(__FILE__).gsub('.', '_')

r.expr([dbName]).set_difference(r.db_list()).for_each{|row| r.db_create(row)}.run
r.expr([tableName]).set_difference(r.db(dbName).table_list()).for_each{|row| r.db(dbName).table_create(row)}.run
$tbl = r.db(dbName).table(tableName)

$tbl.wait(:wait_for=>"all_replicas_ready").run
$tbl.index_create('a').run rescue nil
$tbl.index_wait('a').run

$expected = [["del_in", nil], ["leave", nil], [nil, "enter"], [nil, "ins_in"]]
class Handler < RethinkDB::Handler
  def initialize
    @state = []
  end
  def on_change(a, b)
    @state << [a, b]
    if @state.size == 4
      $res = @state.sort{|a,b| a.to_json <=> b.to_json}
      if $res != $expected
        raise RuntimeError, "Got #{$res} but expected #{$expected}."
      end
      stop
      EM.stop
    end
  end
  def on_open
    $tbl.get('del_in').delete.run(noreply: true)
    $tbl.get('del_out').delete.run(noreply: true)
    $tbl.get('stay_out').update({b: 1}).run(noreply: true)
    $tbl.get('leave').update({a: 1}).run(noreply: true)
    $tbl.get('enter').update({a: 0}).run(noreply: true)
    $tbl.get('stay_in').update({b: 0}).run(noreply: true)
    $tbl.insert({id: 'ins_in', a: 0}).run(noreply: true)
    $tbl.insert({id: 'ins_out', a: 1}).run(noreply: true)
  end
end

EM.run {
  $tbl.delete.run
  ['del_in', 'leave', 'stay_in'].each {|id|
    $tbl.insert({id: id, a: 0}).run
  }
  ['del_out', 'stay_out', 'enter'].each {|id|
    $tbl.insert({id: id, a: 1}).run
  }
  $h = Handler.new
  $tbl.get_all(0, index: 'a')['id'].changes.em_run($h)
}

"Success!"
