# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./rethinkdb')
require 'rethinkdb.rb'
require 'test/unit'
$port_base = ARGV[0].to_i # 0 if none given
class DetTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  def rdb
    @@c.use('test')
    r.table('tbl')
  end
  @@c = RethinkDB::Connection.new('localhost', $port_base + 28015)
  def c; @@c; end
  def server_data; rdb.order_by(:id).run.to_a; end

  def test__init
    begin
      r.db('test').create_table('tbl').run
    rescue
    end
    $data = (0...10).map{|x| {'id' => x}}
    rdb.delete.run
    rdb.insert($data).run
  end
end
