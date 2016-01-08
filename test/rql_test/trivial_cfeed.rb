require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

r.table_create('test').run rescue nil
r.table('test').wait.run
r.table('test').index_create('a').run rescue nil
r.table('test').index_wait('a').run

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
    r.table('test').get('del_in').delete.run(noreply: true)
    r.table('test').get('del_out').delete.run(noreply: true)
    r.table('test').get('stay_out').update({b: 1}).run(noreply: true)
    r.table('test').get('leave').update({a: 1}).run(noreply: true)
    r.table('test').get('enter').update({a: 0}).run(noreply: true)
    r.table('test').get('stay_in').update({b: 0}).run(noreply: true)
    r.table('test').insert({id: 'ins_in', a: 0}).run(noreply: true)
    r.table('test').insert({id: 'ins_out', a: 1}).run(noreply: true)
  end
end

$queries = [r.table('test').get_all(0, index: 'a'),
            r.table('test').between(0, 0.5, index: 'a')]
EM.run {
  r.table('test').delete.run
  ['del_in', 'leave', 'stay_in'].each {|id|
    r.table('test').insert({id: id, a: 0}).run
  }
  ['del_out', 'stay_out', 'enter'].each {|id|
    r.table('test').insert({id: id, a: 1}).run
  }
  $h = Handler.new
  r.table('test').get_all(0, index: 'a')['id'].changes.em_run($h)
}

"Success!"
