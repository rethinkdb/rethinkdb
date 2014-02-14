#!/usr/local/bin/ruby
$LOAD_PATH.unshift '../../drivers/ruby/lib'
$LOAD_PATH.unshift '../../build/drivers/ruby/rdb_protocol'
require 'pp'
require 'rethinkdb'
include RethinkDB::Shortcuts

$port ||= ARGV[0].to_i
ARGV = []
$c = r.connect(port: $port).repl

$run_exc = RethinkDB::RqlRuntimeError
$comp_exc = RethinkDB::RqlCompileError

require 'test/unit'
class ClientTest < Test::Unit::TestCase
  def setup
    r.db_create('test').run rescue nil
    r.table_create('test').run rescue nil
    $tbl = r.table('test')
    $tbl.delete.run
  end

  def test_empty
    assert_equal(1, 2)
  end
end


