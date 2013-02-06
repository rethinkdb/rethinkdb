# -*- coding: utf-8 -*-
# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
require 'rethinkdb.rb'
require 'test/unit'
$port_base ||= ARGV[0].to_i # 0 if none given
$c = RethinkDB::Connection.new('localhost', $port_base + 28015 + 1)

class BacktraceTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts

  def test_map_concatmap
    assert_equal([1, 1, 2],
                 bt{r.concatmap(r.map([1], r.func([], 1)),
                                r.func([], [0, 1, r.error("a")]))})
  end

  def bt
    begin
      yield.run
    rescue Exception => e
      e.to_str.match(/\nBacktrace: (\[[^\]]+\])/) ? eval($1) : []
    end
  end

  def setup
    begin
      r.db_create('test').run
    rescue Exception => e
    end
    begin
      r.db('test').table_create('tbl').run
    rescue Exception => e
    end

    tbl.delete.run
    tbl.insert(data).run
  end

  def tbl
    r.db('test').table('tbl')
  end

  def c; $c; end

  def id_sort x
    x.sort_by do |y|
      y['id']
    end
  end

  def data
    @data ||= (0..9).map do |i|
      {'id' => i, 'num' => i, 'name' => i.to_s}
    end
  end
end

