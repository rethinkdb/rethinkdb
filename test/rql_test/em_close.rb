require 'set'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

def sanitize x
  return x.instance_eval("@token") if x.is_a?(RethinkDB::QueryHandle)
  return x.map{|y| sanitize(y)} if x.is_a?(Array)
  return x
end

r.table_create('test').run rescue nil
r.table('test').delete.run
class Accumulator < RethinkDB::Handler
  attr_accessor :acc
  def initialize
    @acc = []
    @seen = @opened = 0
  end
  def on_open(handle)
    @opened += 1
    if @opened == 2
      r.table('test').insert({id: 0}).run(noreply: true)
    end
    @acc << [:on_open, handle]
  end
  def on_close(*args)
    @acc << [:on_close, *args]
  end
  def on_val(val, handle)
    @acc << [:on_val, val, handle]
    @seen += 1
    if handle == $f2
      $f2.close
      r.table('test').insert({id: 1}).run(noreply: true)
    elsif @seen == 3
      stop
      r.table('test').insert({id: 2}).run(noreply: true)
    end
  end
  def on_state(*args)
    @acc << [:on_state, *args]
  end
end

$a = Accumulator.new
EM.run {
  $f1 = r.table('test').changes.merge({f: 1}).em_run($a)
  $f2 = r.table('test').changes.merge({f: 2}).em_run($a)
  EM.defer {
    while !$f1.closed? || !$f2.closed?
      sleep 0.1
    end
    EM.stop
  }
}

$expected = [[:on_open, $f1],
             [:on_state, "ready", $f1],
             [:on_open, $f2],
             [:on_state, "ready", $f2],
             [:on_val, {"f"=>1, "new_val"=>{"id"=>0}, "old_val"=>nil}, $f1],
             [:on_val, {"f"=>2, "new_val"=>{"id"=>0}, "old_val"=>nil}, $f2],
             [:on_close, $f2],
             [:on_val, {"f"=>1, "new_val"=>{"id"=>1}, "old_val"=>nil}, $f1]]

if Set.new($a.acc) != Set.new($expected)
  raise RuntimeError, "Unexpected output:\n" + sanitize($a.acc).inspect +
    "\nVS:\n" + sanitize($expected).inspect
end
