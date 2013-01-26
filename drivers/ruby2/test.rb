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

  def test_without
    assert_equal({ "b" => 2 }, r(:a => 1, :b => 2, :c => 3).without(:a, :c).run)
    assert_equal({ "a" => 1 }, r(:a => 1).without(:b).run)
    assert_equal({}, r({}).without(:b).run)
  end

  def test_arr_ordering
    assert_equal(true, r.lt([1, 2, 3], [1, 2, 3, 4]).run)
    assert_equal(false, r.lt([1, 2, 3], [1, 2, 3]).run)
  end

  def test_junctions
    assert_equal(false, r.any(false).run)
    assert_equal(true, r.any(true, false).run)
    assert_equal(true, r.any(false, true).run)
    assert_equal(true, r.any(false, false, true).run)

    assert_equal(false, r.all(false).run)
    assert_equal(false, r.all(true, false).run)
    assert_equal(true, r.all(true, true).run)
    assert_equal(true, r.all(true, true, true).run)

    assert_raise(RuntimeError) { r.all(true, 3).run }
    assert_raise(RuntimeError) { r.any(4, true).run }
  end

  def test_not
    assert_equal(false, r.not(true).run)
    assert_equal(true, r.not(false).run)
    assert_raise(RuntimeError) { r.not(3).run }
  end

  # TODO: more here
  def test_scopes
    assert_equal([1, 1], r(1).do{|a| [a, a]}.run)
  end

  def test_do
    assert_equal(3, r(3).do {|x| x}.run)
    assert_raise(RuntimeError){r(3).do {|x,y| x}.run}
    assert_equal(3, r.do(3,4) {|x,y| x}.run)
    assert_equal(4, r.do(3,4) {|x,y| y}.run)
    assert_equal(7, r.do(3,4) {|x,y| x+y}.run)
  end

  def test_if
    assert_equal(3, r.branch(true, 3, 4).run)
    assert_equal(5, r.branch(false, 4, 5).run)
    assert_equal("foo", r.branch(r.eq(3, 3), "foo", "bar").run)
    assert_raise(RuntimeError) { r.branch(5, 1, 2).run }
  end

  def test_array_python # from python tests
    assert_equal([2], r([]).append(2).run.to_a)
    assert_equal([1, 2], r([1]).append(2).run.to_a)
    assert_raise(RuntimeError) { r(2).append(0).run.to_a }

    assert_equal([1, 2], r.union([1], [2]).run.to_a)
    assert_equal([1, 2], r.union([1, 2], []).run.to_a)
    assert_raise(RuntimeError) { r.union(1, [1]).run }
    assert_raise(RuntimeError) { r.union([1], 1).run }

    arr = (0...10).collect { |x| x }
    assert_equal(arr[(0...3)], r(arr)[(0...3)].run.to_a)
    assert_raise(ArgumentError){r(arr)[(0...0)].run.to_a}
    assert_equal(arr[(5...15)], r(arr)[(5...15)].run.to_a)
    assert_equal(arr[(5...-3)], r(arr)[(5...-3)].run.to_a)
    assert_equal(arr[(-5...-3)], r(arr)[(-5...-3)].run.to_a)
    assert_equal(arr[(0..3)], r(arr)[(0..3)].run.to_a)
    assert_equal(arr[(0..0)], r(arr)[(0..0)].run.to_a)
    assert_equal(arr[(5..15)], r(arr)[(5..15)].run.to_a)
    assert_equal(arr[(5..-3)], r(arr)[(5..-3)].run.to_a)
    assert_equal(arr[(-5..-3)], r(arr)[(-5..-3)].run.to_a)

    assert_raise(RuntimeError) { r(1)[(0...1)].run.to_a }
    assert_raise(RuntimeError) { r(arr)[(0.5...1)].run.to_a }
    assert_raise(RuntimeError) { r(1)[(0...1.01)].run.to_a }
    assert_raise(RuntimeError) { r(1)[(5...3)].run.to_a }

    assert_equal(arr[(5..-1)], r(arr)[(5..-1)].run.to_a)
    assert_equal(arr[(0...7)], r(arr)[(0...7)].run.to_a)
    assert_equal(arr[(0...-2)], r(arr)[(0...-2)].run.to_a)
    assert_equal(arr[(-2..-1)], r(arr)[(-2..-1)].run.to_a)
    assert_equal(arr[(0..-1)], r(arr)[(0..-1)].run.to_a)

    assert_equal(3, r(arr)[3].run)
    assert_equal(9, r(arr)[-1].run)
    assert_raise(RuntimeError) { r(0)[0].run }
    assert_raise(ArgumentError) { r(arr)[0.1].run }
    assert_raise(RuntimeError) { r([0])[1].run }

    assert_equal(0, r([]).count.run)
    assert_equal(arr.length, r(arr).count.run)
    assert_raise(RuntimeError) { r(0).count.run }
  end

  # from python tests
  def test_stream_fancy
    assert_equal([], r.limit([], 0).run)
    assert_equal([], r.limit([1, 2], 0).run)
    assert_equal([1], r.limit([1, 2], 1).run)
    assert_equal([1, 2], r.limit([1, 2], 5).run)
    assert_raise(RuntimeError) { r.limit([], -1).run }

    assert_equal([], r.skip([], 0).run)
    assert_equal([], r.skip([1, 2], 5).run)
    assert_equal([1, 2], r.skip([1, 2], 0).run)
    assert_equal([2], r.skip([1, 2], 1).run)

    assert_equal([], r.distinct([]).run)
    assert_equal([1, 2, 3], r.distinct(([1, 2, 3] * 10)).run)
    assert_equal([1, 2, 3], r.distinct([1, 2, 3, 2]).run)
    assert_equal([false, true, 2.0], r.distinct([true, 2, false, 2]).run)
  end

  def test_ordering
    docs = (0..9).map { |n| { "id" => ((100 + n)), "a" => (n), "b" => ((n % 3)) } }
    assert_equal(docs.sort_by { |x| x["a"] }, r(docs).orderby(:s).run)
    assert_equal(docs.sort_by { |x| x["a"] }.reverse, r(docs).orderby('-a').run)
    assert_equal(docs.select { |x| (x["b"] == 0) }.sort_by { |x| x["a"] },
                 r(docs).filter{|x| x[:b].eq 0}.orderby('+a').run)
    assert_equal(docs.select { |x| (x["b"] == 0) }.sort_by { |x| x["a"] }.reverse,
                 r(docs).filter{|x| x[:b].eq 0}.orderby('-a').run)
  end

  # +,-,%,*,/,<,>,<=,>=,eq,ne,any,all
  def test_ops
    assert_equal(8, (r(5) + 3).run)
    assert_equal(8, r(5).add(3).run)
    assert_equal(8, r.add(5, 3).run)
    assert_equal(8, r.add(2, 3, 3).run)

    assert_equal(2, (r(5) - 3).run)
    assert_equal(2, r(5).sub(3).run)
    assert_equal(2, r.sub(5, 3).run)

    assert_equal(2, (r(5) % 3).run)
    assert_equal(2, r(5).mod(3).run)
    assert_equal(2, r.mod(5, 3).run)

    assert_equal(15, (r(5) * 3).run)
    assert_equal(15, r(5).mul(3).run)
    assert_equal(15, r.mul(5, 3).run)

    assert_equal(5, (r(15) / 3).run)
    assert_equal(5, r(15).div(3).run)
    assert_equal(5, r.div(15, 3).run)

    assert_equal(false, r.lt(3, 2).run)
    assert_equal(false, r.lt(3, 3).run)
    assert_equal(true, r.lt(3, 4).run)
    assert_equal(false, r(3).lt(2).run)
    assert_equal(false, r(3).lt(3).run)
    assert_equal(true, r(3).lt(4).run)
    assert_equal(false, (r(3) < 2).run)
    assert_equal(false, (r(3) < 3).run)
    assert_equal(true, (r(3) < 4).run)

    assert_equal(false, r.le(3, 2).run)
    assert_equal(true, r.le(3, 3).run)
    assert_equal(true, r.le(3, 4).run)
    assert_equal(false, r(3).le(2).run)
    assert_equal(true, r(3).le(3).run)
    assert_equal(true, r(3).le(4).run)
    assert_equal(false, (r(3) <= 2).run)
    assert_equal(true, (r(3) <= 3).run)
    assert_equal(true, (r(3) <= 4).run)

    assert_equal(true, r.gt(3, 2).run)
    assert_equal(false, r.gt(3, 3).run)
    assert_equal(false, r.gt(3, 4).run)
    assert_equal(true, r(3).gt(2).run)
    assert_equal(false, r(3).gt(3).run)
    assert_equal(false, r(3).gt(4).run)
    assert_equal(true, (r(3) > 2).run)
    assert_equal(false, (r(3) > 3).run)
    assert_equal(false, (r(3) > 4).run)

    assert_equal(true, r.ge(3, 2).run)
    assert_equal(true, r.ge(3, 3).run)
    assert_equal(false, r.ge(3, 4).run)
    assert_equal(true, r(3).ge(2).run)
    assert_equal(true, r(3).ge(3).run)
    assert_equal(false, r(3).ge(4).run)
    assert_equal(true, (r(3) >= 2).run)
    assert_equal(true, (r(3) >= 3).run)
    assert_equal(false, (r(3) >= 4).run)

    assert_equal(false, r.eq(3, 2).run)
    assert_equal(true, r.eq(3, 3).run)

    assert_equal(true, r.ne(3, 2).run)
    assert_equal(false, r.ne(3, 3).run)

    assert_equal(true, r.all(true, true, true).run)
    assert_equal(false, r.all(true, false, true).run)
    assert_equal(true, r.and(true, true, true).run)
    assert_equal(false, r.and(true, false, true).run)
    assert_equal(true, r(true).and(true).run)
    assert_equal(false, r(true).and(false).run)

    assert_equal(false, r.any(false, false, false).run)
    assert_equal(true, r.any(false, true, false).run)

    assert_equal(false, r.or(false, false, false).run)
    assert_equal(true, r.or(false, true, false).run)
    assert_equal(false, r(false).or(false).run)
    assert_equal(true, r(true).or(false).run)
  end

  # TABLE
  def test_easy_read
    assert_equal(id_sort(tbl.run.to_a), data)
    assert_equal(id_sort(r.db("test").table("tbl").run.to_a), data)
  end

  # IF, JSON, ERROR
  def test_error
    query = r.branch((r.add(1, 2) >= 3), r([1,2,3]), r.error("unreachable"))
    query_err = r.branch((r.add(1, 2) > 3), r([1,2,3]), r.error("reachable"))
    assert_equal([1, 2, 3], query.run)
    assert_raise(RuntimeError) { assert_equal(query_err.run) }
  end

  # BOOL, JSON_NULL, ARRAY, ARRAYTOSTREAM
  def test_array
    assert_equal([true, false, nil], r([true, false, nil]).run)
    assert_equal([true, false, nil], r([true, false, nil]).coerce("array").run.to_a)
  end

  # OBJECT, GETBYKEY
  def test_getbykey
    query = r("obj" => (tbl.get(0)))
    query2 = r("obj" => (tbl.get(0)))
    assert_equal(data[0], query.run["obj"])
    assert_equal(data[0], query2.run["obj"])
  end

  # MAP, FILTER, GETATTR, IMPLICIT_GETATTR, STREAMTOARRAY
  def test_map
    assert_equal([1, 2], r([{ :id => 1 }, { :id => 2 }]).map { |row| row[:id] }.run.to_a)
    assert_raise(RuntimeError) { r(1).map { }.run.to_a }
    assert_equal([data[1]], tbl.filter("num" => 1).run.to_a)
    query = tbl.order_by(:id).map do |outer_row|
      tbl.filter { |row| (row[:id] < outer_row[:id]) }.coerce("array")
    end
    assert_equal(data[(0..1)], id_sort(query.run.to_a[2]))
  end

  # REDUCE, HASATTR, IMPLICIT_HASATTR
  def test_reduce
    # TODO: Error checking for reduce
    assert_equal(6, r([1, 2, 3]).reduce(0) { |a, b| (a + b) }.run)
    assert_raise(RuntimeError) { r(1).reduce(0) { 0 }.run }

    # assert_equal(  tbl.map{|row| row['id']}.reduce(0){|a,b| a+b}.run,
    #              data.map{|row| row['id']}.reduce(0){|a,b| a+b})

    assert_equal(data.map{ true }, tbl.map { |r| r.contains(:id) }.run.to_a)
    assert_equal(data.map{ false }, tbl.map { |row| r.not(row.contains("id")) }.run.to_a)
    assert_equal(data.map{ false }, tbl.map { |row| r.not(row.contains("id")) }.run.to_a)
    assert_equal(data.map{ false }, tbl.map { |row| r.not(row.contains("id")) }.run.to_a)
    assert_equal(data.map{ true }, tbl.map { |r| r.contains(:id) }.run.to_a)
    assert_equal(data.map{ false }, tbl.map { |r| r.contains("id").not }.run.to_a)
  end

  # assert_equal(  tbl.map{|row| row['id']}.reduce(0){|a,b| a+b}.run,
  #              data.map{|row| row['id']}.reduce(0){|a,b| a+b})
  # FILTER
  def test_filter
    assert_equal([1, 2], r([1, 2, 3]).filter { |x| (x < 3) }.run.to_a)
    assert_raise(RuntimeError) { r(1).filter { true }.run.to_a }
    query_5 = tbl.filter { |r| r[:name].eq("5") }
    assert_equal([data[5]], query_5.run.to_a)
    query_2345 = tbl.filter { |row| r.and((row[:id] >= 2), (row[:id] <= 5)) }
    query_234 = query_2345.filter { |row| row[:num].ne(5) }
    query_23 = query_234.filter { |row| r.any(row[:num].eq(2), row[:num].eq(3)) }
    assert_equal(data[(2..5)], id_sort(query_2345.run.to_a))
    assert_equal(data[(2..4)], id_sort(query_234.run.to_a))
    assert_equal(data[(2..3)], id_sort(query_23.run.to_a))
    assert_equal(data[(0...5)], id_sort(tbl.filter { |r| (r[:id] < 5) }.run.to_a))
  end

  def test_joins
    db_name = rand((2 ** 64)).to_s
    tbl_name = rand((2 ** 64)).to_s
    r.db_create(db_name).run
    r.db(db_name).table_create((tbl_name + "1")).run
    r.db(db_name).table_create((tbl_name + "2")).run
    tbl1 = r.db(db_name).table((tbl_name + "1"))
    tbl2 = r.db(db_name).table((tbl_name + "2"))

    assert_equal({ "inserted" => 10},
                 tbl1.insert((0...10).map { |x| { :id => (x), :a => ((x * 2)) } }).run)
    assert_equal({ "inserted" => 10},
                 tbl2.insert((0...10).map { |x| { :id => (x), :b => ((x * 100)) } }).run)

    tbl1.eq_join(:a, tbl2).run.each do |pair|
      assert_equal(pair["right"]["id"], pair["left"]["a"])
    end

    assert_equal(tbl1.inner_join(tbl2){ |l, r| l[:a].eq(r[:id]) }.run.to_a.sort_by do |x|
      x["left"]["id"]
    end, tbl1.eq_join(:a, tbl2).run.to_a.sort_by { |x| x["left"]["id"] })

    assert_equal((tbl1.eq_join(:a, tbl2).run.to_a +
                  tbl1.filter{|row| tbl2.get(row[:a]).eq(nil)}.map {|row|
      {:left => row}}.run.to_a).sort_by {|x| x['left']['id']},
        tbl1.outer_join(tbl2) {|l,r|
        l[:a].eq(r[:id])}.run.to_a.sort_by {|x| x['left']['id']})

    assert_equal(tbl1.outer_join(tbl2) {
                   |l,r| l[:a].eq(r[:id])}.zip.run.to_a.sort_by {|x| x['a']},
                 [{"b"=>0, "a"=>0, "id"=>0},
                  {"b"=>200, "a"=>2, "id"=>2},
                  {"b"=>400, "a"=>4, "id"=>4},
                  {"b"=>600, "a"=>6, "id"=>6},
                  {"b"=>800, "a"=>8, "id"=>8},
                  {"a"=>10, "id"=>5},
                  {"a"=>12, "id"=>6},
                  {"a"=>14, "id"=>7},
                  {"a"=>16, "id"=>8},
                  {"a"=>18, "id"=>9}])
  ensure
    r.db_drop(db_name).run
  end

  def test_random_insert_regressions
    assert_raise(RuntimeError){tbl.insert(true).run}
    assert_not_nil(tbl.insert([true, true]).run['errors'])
  end

  def test_too_big_key
    assert_not_nil(tbl.insert({ :id => (("a" * 1000)) }).run["first_error"])
  end

  def test_key_generation
    assert_equal(data, tbl.order_by(:id).run.to_a)
    res = tbl.insert([[], {}, { :a => 1 }, { :id => -1, :a => 2 }]).run
    assert_equal(1, res["errors"])
    assert_equal(3, res["inserted"])
    assert_equal(2, res["generated_keys"].length)
    assert_equal(1, tbl.get(res["generated_keys"][0]).delete.run["deleted"])
    assert_equal(2,tbl.filter{|x| (x[:id] < 0)|(x[:id] > 1000)}.delete.run["deleted"])
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  # SLICE
  def test_slice
    arr = [0, 1, 2, 3, 4, 5]
    assert_equal(1, r(arr)[1].run)
    assert_equal(r(arr)[(2..-1)].run.to_a, r(arr)[(2...6)].run.to_a)
    assert_equal(r(arr)[(2..4)].run.to_a, r(arr)[(2...5)].run.to_a)
    assert_raise(ArgumentError) { r(arr)[(2...0)].run.to_a }
  end

  def test_mapmerge
    assert_equal({ "a" => 1, "b" => 2 }, r(:a => 1).merge(:b => 2).run)
    assert_equal({ "a" => 2 }, r.merge({ :a => 1 }, :a => 2).run)
  end

  # ORDERBY, MAP
  def test_order_by
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_equal(data, tbl.order_by("id").run.to_a)
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_equal(data, tbl.order_by(:"+id").run.to_a)
    assert_equal(data.reverse, tbl.order_by(:"-id").run.to_a)
    query = tbl.map { |x|
      r(:id => (x[:id]), :num => (x[:id].mod(2)))
    }.order_by(:num, '-id')
    want = data.map { |o| o["id"] }.sort_by { |n| (((n % 2) * data.length) - n) }
    assert_equal(want, query.run.to_a.map { |o| o["id"] })
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
