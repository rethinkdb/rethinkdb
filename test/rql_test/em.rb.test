require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

class DefaultHandler < RethinkDB::Handler
  attr_accessor :state
  def initialize; @state = []; end
  def on_error(err)
    if err != RethinkDB::ReqlRuntimeError.new("Connection is closed.")
      @state << [:err, err]
    end
  end
  def on_val(val)
    @state << [:val, val]
  end
end

class ValTypeHandler < RethinkDB::Handler
  attr_accessor :state
  def initialize; @state = []; end
  def on_error(err)
    if err != RethinkDB::ReqlRuntimeError.new("Connection is closed.")
      @state << [:err, err]
    end
  end
  def on_atom(val)
    @state << [:atom, val]
  end
  def on_stream_val(val)
    @state << [:stream_val, val]
  end
end

class ValTypeHandler2 < ValTypeHandler
  def on_array(arr)
    arr.each{|x| on_stream_val(x)}
  end
end

class ChangeOnlyHandler < RethinkDB::Handler
  attr_accessor :state
  def initialize; @state = []; end
  def on_error(err)
    if err != RethinkDB::ReqlRuntimeError.new("Connection is closed.")
      @state << [:err, err]
    end
  end
  def on_change(old_val, new_val)
    @state << [:change, old_val, new_val]
  end
end

class ChangeHandler < ValTypeHandler2
  def on_change(old_val, new_val)
    @state << [:change, old_val, new_val]
  end
end

class CleverChangeHandler < ChangeHandler
  def on_state(state)
    @state << [:state, state]
  end
  def on_initial_val(initial_val)
    @state << [:initial_val, initial_val]
  end
end

def run1(x, handler)
  x.em_run(handler)
end
def run2(x, handler)
  x.em_run($c, handler)
end
def run3(x, handler)
  x.em_run($c, {durability: 'soft'}, handler)
end
def run4(x, handler)
  x.em_run({durability: 'soft'}, handler)
end
def brun1(x, handler)
  handler.is_a?(Proc) ? x.em_run(&handler) : run1(x, handler)
end
def brun2(x, handler)
  handler.is_a?(Proc) ? x.em_run($c, &handler) : run2(x, handler)
end
def brun3(x, handler)
  handler.is_a?(Proc) ? x.em_run($c, {durability: 'soft'}, &handler) : run3(x, handler)
end
def brun4(x, handler)
  handler.is_a?(Proc) ? x.em_run({durability: 'soft'}, &handler) : run4(x, handler)
end
$runners = [method(:run1), method(:run2), method(:run3), method(:run4),
            method(:brun1), method(:brun2), method(:brun3), method(:brun4)]

r.table_create('test').run rescue nil
r.table('test').reconfigure(shards: 2, replicas: 1).run
r.table('test').wait(:wait_for=>"all_replicas_ready").run
r.table('test').delete.run
r.table('test').insert({id: 0}).run
EM.run {
  $lambda_state = []
  $lambda = lambda {|err, row|
    if err != RethinkDB::ReqlRuntimeError.new("Connection is closed.")
      $lambda_state << [err, row]
    end
  }
  $handlers = [DefaultHandler.new, ValTypeHandler.new, ValTypeHandler2.new,
               ChangeOnlyHandler.new, ChangeHandler.new, CleverChangeHandler.new]

  $runners.each {|runner|
    ($handlers + [$lambda]).each {|handler|
      runner.call(r.table('test').get(0), handler)
      runner.call(r.table('test').get_all(0).coerce_to('array'), handler)
      runner.call(r.table('test'), handler)
      runner.call(r.table('fake'), handler)
      runner.call(r.table('test').get(0).changes(include_initial: true), handler)
      runner.call(r.table('test').changes, handler)
    }
  }
  EM.defer {
    sleep 1
    r.table('test').insert({id: 1}).run
    r.table('test').get(0).update({a: 1}).run
  }
  EM.defer(proc{sleep 2}, proc{EM.stop})
}

def sub_canon x
  case x
  when Array
    x.map{|y| sub_canon(y)}.sort{|a,b| a.to_json <=> b.to_json}
  when Hash
    sub_canon(x.to_a)
  when RuntimeError
    "error"
  else
    x
  end
end
def canon x
  x.map{|pair| [sub_canon(pair[0]), sub_canon(pair[1])]}.sort {|a,b|
    a.to_json <=> b.to_json
  }
end
$res = ($handlers.map{|x| [x.class, canon(x.state)]} +
        [[Proc, canon($lambda_state)]])

$expected = [[DefaultHandler,
              [[:err, "error"],
               [:val, [["id", 0]]],
               [:val, [["id", 0]]],
               [:val, [["id", 0]]],
               [:val, [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [:val, [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [:val, [["new_val", [["id", 0]]]]],
               [:val, [["new_val", [["id", 1]]], ["old_val", nil]]]]],
             [ValTypeHandler,
              [[:atom, [["id", 0]]],
               [:atom, [[["id", 0]]]],
               [:err, "error"],
               [:stream_val, [["id", 0]]],
               [:stream_val,
                [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [:stream_val,
                [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [:stream_val, [["new_val", [["id", 0]]]]],
               [:stream_val, [["new_val", [["id", 1]]], ["old_val", nil]]]]],
             [ValTypeHandler2,
              [[:atom, [["id", 0]]],
               [:err, "error"],
               [:stream_val, [["id", 0]]],
               [:stream_val, [["id", 0]]],
               [:stream_val,
                [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [:stream_val,
                [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [:stream_val, [["new_val", [["id", 0]]]]],
               [:stream_val, [["new_val", [["id", 1]]], ["old_val", nil]]]]],
             [ChangeOnlyHandler,
              [[:change, [["id", 0]]],
               [:change, [["id", 0]]],
               [:change, nil],
               [:err, "error"]]],
             [ChangeHandler,
              [[:atom, [["id", 0]]],
               [:change, [["id", 0]]],
               [:change, [["id", 0]]],
               [:change, nil],
               [:err, "error"],
               [:stream_val, [["id", 0]]],
               [:stream_val, [["id", 0]]],
               [:stream_val, [["new_val", [["id", 0]]]]]]],
             [CleverChangeHandler,
              [[:atom, [["id", 0]]],
               [:change, [["id", 0]]],
               [:change, [["id", 0]]],
               [:change, nil],
               [:err, "error"],
               [:initial_val, [["id", 0]]],
               [:state, "initializing"],
               [:state, "ready"],
               [:state, "ready"],
               [:stream_val, [["id", 0]]],
               [:stream_val, [["id", 0]]]]],
             [Proc,
              [["error", nil],
               [nil, [["id", 0]]],
               [nil, [["id", 0]]],
               [nil, [["id", 0]]],
               [nil, [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [nil, [["new_val", [["a", 1], ["id", 0]]], ["old_val", [["id", 0]]]]],
               [nil, [["new_val", [["id", 0]]]]],
               [nil, [["new_val", [["id", 1]]], ["old_val", nil]]]]]]
$expected = $expected.map{|arr| [arr[0], arr[1].flat_map{|x| [x]*$runners.size}]}

if $res != $expected
  raise RuntimeError, "Unexpected output:\n" + PP.pp($res, "")
end
