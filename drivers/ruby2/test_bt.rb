# -*- coding: utf-8 -*-
# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
require 'rethinkdb.rb'
require 'test/unit'
$port_base ||= ARGV[0].to_i # 0 if none given
$c = RethinkDB::Connection.new('localhost', $port_base + 28015 + 1)

class BacktraceTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts

  def test_slice
    assert_bt([2]) { r([1,2,3,4,5]).slice(0, r.error("a")) }
    assert_bt([1]) { r([1,2,3,4,5]).slice(r.error("b"), r.error("a")) }

    assert_bt([2]) { r([1,2,3,4,5]).slice(0, tbl).run }
    assert_bt([2]) { r([1,2,3,4,5]).slice(0, "a").run }
    assert_bt([1]) { r([1,2,3,4,5]).slice(tbl, 0).run }
    assert_bt([1]) { r([1,2,3,4,5]).slice("a", 0).run }

    assert_bt([0, 0, 2]) { r([1,2,3]).slice(0,:a).slice(0,0).slice(0,0).run }
    assert_bt([0, 2]) { r([1,2,3]).slice(0,0).slice(0,:a).slice(0,0).run }
    assert_bt([0, 0, 0]) { r.error('a').slice(0,:a).slice(0,0).slice(0,0).run }
  end

  def test_map_concatmap
    assert_equal([1, 1, 2],
                 bt{r.concatmap(r.map([1], r.func([], 1)),
                                r.func([], [0, 1, r.error("a")]))})
    assert_equal([1, 1, 2, 0],
                 bt{r.concatmap(r.map([1], r.func([], 1)),
                                r.func([], [0, 1, [r.error("a")]]))})
    assert_equal([0, 1, 1],
                 bt{r.concatmap(r.map([1], r.func([], r.error("b"))),
                                r.func([], [0, 1, [r.error("a")]]))})
  end

  def assert_bt(val, &b)
    assert_equal(val, bt(&b))
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

