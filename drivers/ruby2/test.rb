# -*- coding: utf-8 -*-
# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
require 'rethinkdb.rb'
require 'test/unit'
$port_base ||= ARGV[0].to_i # 0 if none given
$c = RethinkDB::Connection.new('localhost', $port_base + 28015 + 1)
class ClientTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts

  def test_numops
    assert_raise(RuntimeError) { r.div(1, 0).run }
    assert_raise(RuntimeError) { r.div(0, 0).run }
    assert_raise(RuntimeError) { r.mul(1.0e+200, 1.0e+300).run }
    assert_equal(1.0e+200, r.mul(1.0e+100, 1.0e+100).run)
  end

  # from python tests
  def test_cmp
    assert_equal(true, r.eq(3, 3).run)
    assert_equal(false, r.eq(3, 4).run)

    assert_equal(false, r.ne(3, 3).run)
    assert_equal(true, r.ne(3, 4).run)

    assert_equal(true, r.gt(3, 2).run)
    assert_equal(false, r.gt(3, 3).run)

    assert_equal(true, r.ge(3, 3).run)
    assert_equal(false, r.ge(3, 4).run)

    assert_equal(false, r.lt(3, 3).run)
    assert_equal(true, r.lt(3, 4).run)

    assert_equal(true, r.le(3, 3).run)
    assert_equal(true, r.le(3, 4).run)
    assert_equal(false, r.le(3, 2).run)

    assert_equal(false, r.eq(1, 1, 2).run)
    assert_equal(true, r.eq(5, 5, 5).run)

    assert_equal(true, r.lt(3, 4).run)
    assert_equal(true, r.lt(3, 4, 5).run)
    assert_equal(false, r.lt(5, 6, 7, 7).run)

    assert_equal(true, r.eq("asdf", "asdf").run)
    assert_equal(false, r.eq("asd", "asdf").run)
    assert_equal(true, r.lt("a", "b").run)

    assert_equal(true, r.eq(true, true).run)
    assert_equal(true, r.lt(false, true).run)

    assert_equal(true, r.lt([], false, true, nil, 1, {}, "").run)
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
      { 'id' => i, 'num' => i, 'name' => i.to_s }
    end
  end
end
