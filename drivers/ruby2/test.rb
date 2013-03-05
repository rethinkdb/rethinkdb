# -*- coding: utf-8 -*-
# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
require 'rethinkdb.rb'
require 'test/unit'
require 'pp'

$port_base ||= ARGV[0].to_i # 0 if none given
$c = RethinkDB::Connection.new('localhost', $port_base + 28015 + 1)

class ClientTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts

  def test_groupby
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_equal({ "replaced" => 10 },
                 tbl.update { |row| { :gb => ((row[:id] % 3)) } }.run($c))
    counts = tbl.group_by(:gb, r.count).run($c).sort_by { |x| x["group"] }
    sums = tbl.group_by(:gb, r.sum(:id)).run($c).sort_by { |x| x["group"] }
    avgs = tbl.group_by(:gb, r.avg(:id)).run($c).sort_by { |x| x["group"] }
    avgs.each_index do |i|
      assert_equal(avgs[i]["reduction"],
                   (sums[i]["reduction"].to_f / counts[i]["reduction"]))
    end
  end

  def test_coerce_to
    assert_equal("1", r(1).coerce_to("string").run($c))
    assert_equal([["a", 1.0], ["b", 2.0]],
                 r({:a => 1, :b => 2}).coerce_to("array").run($c))
    assert_raise(RuntimeError) {r(1).coerce_to("datum").run($c)}
    assert_equal(2, r.db('test').table('tbl').coerce_to("array")[-3..-2].run($c).size)
    assert_raise(RuntimeError) {r.db('test').table('tbl')[-3..-2].run($c)}
    assert_equal({"a"=>1.0, "b"=>2.0},
                 r([["a", 1], ["b", 2]]).coerce_to("object").run($c))
    assert_raise(RuntimeError) {r([["a", 1], ["a", 2]]).coerce_to("object").run($c)}
  end

  def test_typeof
    assert_equal("ARRAY", r([]).typeof.run($c))
    assert_equal("BOOL", r(true).typeof.run($c))
    assert_equal("NULL", r(nil).typeof.run($c))
    assert_equal("NUMBER", r(1).typeof.run($c))
    assert_equal("OBJECT", r({}).typeof.run($c))
    assert_equal("STRING", r("").typeof.run($c))
    assert_equal("DB", r.db('test').typeof.run($c))
    assert_equal("TABLE", r.db('test').table('tbl').typeof.run($c))
    assert_equal("STREAM", r.db('test').table('tbl').map{|x| x}.typeof.run($c))
    assert_equal("SELECTION", r.db('test').table('tbl').filter{|x| true}.typeof.run($c))
    assert_equal("SINGLE_SELECTION", r.db('test').table('tbl').get(0).typeof.run($c))
    assert_equal("FUNCTION", r(lambda {}).typeof.run($c))
  end

  def test_numops
    assert_raise(RuntimeError) {r.div(1, 0).run($c)}
    assert_raise(RuntimeError) {r.div(0, 0).run($c)}
    assert_raise(RuntimeError) {r.mul(1.0e+200, 1.0e+300).run($c)}
    assert_equal(1.0e+200, r.mul(1.0e+100, 1.0e+100).run($c))
  end

  # from python tests
  def test_cmp
    assert_equal(true, r.eq(3, 3).run($c))
    assert_equal(false, r.eq(3, 4).run($c))

    assert_equal(false, r.ne(3, 3).run($c))
    assert_equal(true, r.ne(3, 4).run($c))

    assert_equal(true, r.gt(3, 2).run($c))
    assert_equal(false, r.gt(3, 3).run($c))

    assert_equal(true, r.ge(3, 3).run($c))
    assert_equal(false, r.ge(3, 4).run($c))

    assert_equal(false, r.lt(3, 3).run($c))
    assert_equal(true, r.lt(3, 4).run($c))

    assert_equal(true, r.le(3, 3).run($c))
    assert_equal(true, r.le(3, 4).run($c))
    assert_equal(false, r.le(3, 2).run($c))

    assert_equal(false, r.eq(1, 1, 2).run($c))
    assert_equal(true, r.eq(5, 5, 5).run($c))

    assert_equal(true, r.lt(3, 4).run($c))
    assert_equal(true, r.lt(3, 4, 5).run($c))
    assert_equal(false, r.lt(5, 6, 7, 7).run($c))

    assert_equal(true, r.eq("asdf", "asdf").run($c))
    assert_equal(false, r.eq("asd", "asdf").run($c))
    assert_equal(true, r.lt("a", "b").run($c))

    assert_equal(true, r.eq(true, true).run($c))
    assert_equal(true, r.lt(false, true).run($c))

    assert_equal(true, r.lt([], false, true, nil, 1, {}, "").run($c))
  end

  def test_without
    assert_equal({"b" => 2}, r(:a => 1, :b => 2, :c => 3).without(:a, :c).run($c))
    assert_equal({"a" => 1}, r(:a => 1).without(:b).run($c))
    assert_equal({}, r({}).without(:b).run($c))
  end

  def test_arr_ordering
    assert_equal(true, r.lt([1, 2, 3], [1, 2, 3, 4]).run($c))
    assert_equal(false, r.lt([1, 2, 3], [1, 2, 3]).run($c))
  end

  def test_junctions
    assert_equal(false, r.any(false).run($c))
    assert_equal(true, r.any(true, false).run($c))
    assert_equal(true, r.any(false, true).run($c))
    assert_equal(true, r.any(false, false, true).run($c))

    assert_equal(false, r.all(false).run($c))
    assert_equal(false, r.all(true, false).run($c))
    assert_equal(true, r.all(true, true).run($c))
    assert_equal(true, r.all(true, true, true).run($c))

    assert_raise(RuntimeError) {r.all(true, 3).run($c)}
    assert_raise(RuntimeError) {r.any(4, true).run($c)}
  end

  def test_not
    assert_equal(false, r.not(true).run($c))
    assert_equal(true, r.not(false).run($c))
    assert_raise(RuntimeError) {r.not(3).run($c)}
  end

  # TODO: more here
  def test_scopes
    assert_equal([1, 1], r(1).do{|a| [a, a]}.run($c))
  end

  def test_do
    assert_equal(3, r(3).do {|x| x}.run($c))
    assert_raise(RuntimeError){r(3).do {|x,y| x}.run($c)}
    assert_equal(3, r.do(3,4) {|x,y| x}.run($c))
    assert_equal(4, r.do(3,4) {|x,y| y}.run($c))
    assert_equal(7, r.do(3,4) {|x,y| x+y}.run($c))
  end

  def test_if
    assert_equal(3, r.branch(true, 3, 4).run($c))
    assert_equal(5, r.branch(false, 4, 5).run($c))
    assert_equal("foo", r.branch(r.eq(3, 3), "foo", "bar").run($c))
    assert_raise(RuntimeError) {r.branch(5, 1, 2).run($c)}
  end

  def test_array_python # from python tests
    assert_equal([2], r([]).append(2).run($c).to_a)
    assert_equal([1, 2], r([1]).append(2).run($c).to_a)
    assert_raise(RuntimeError) {r(2).append(0).run($c).to_a}

    assert_equal([1, 2], r.union([1], [2]).run($c).to_a)
    assert_equal([1, 2], r.union([1, 2], []).run($c).to_a)
    assert_raise(RuntimeError) {r.union(1, [1]).run($c)}
    assert_raise(RuntimeError) {r.union([1], 1).run($c)}

    arr = (0...10).collect {|x| x}
    assert_equal(arr[(0...3)], r(arr)[(0...3)].run($c).to_a)
    assert_raise(ArgumentError){r(arr)[(0...0)].run($c).to_a}
    assert_equal(arr[(5...15)], r(arr)[(5...15)].run($c).to_a)
    assert_equal(arr[(5...-3)], r(arr)[(5...-3)].run($c).to_a)
    assert_equal(arr[(-5...-3)], r(arr)[(-5...-3)].run($c).to_a)
    assert_equal(arr[(0..3)], r(arr)[(0..3)].run($c).to_a)
    assert_equal(arr[(0..0)], r(arr)[(0..0)].run($c).to_a)
    assert_equal(arr[(5..15)], r(arr)[(5..15)].run($c).to_a)
    assert_equal(arr[(5..-3)], r(arr)[(5..-3)].run($c).to_a)
    assert_equal(arr[(-5..-3)], r(arr)[(-5..-3)].run($c).to_a)

    assert_raise(RuntimeError) {r(1)[(0...1)].run($c).to_a}
    assert_raise(RuntimeError) {r(arr)[(0.5...1)].run($c).to_a}
    assert_raise(RuntimeError) {r(1)[(0...1.01)].run($c).to_a}
    assert_raise(RuntimeError) {r(1)[(5...3)].run($c).to_a}

    assert_equal(arr[(5..-1)], r(arr)[(5..-1)].run($c).to_a)
    assert_equal(arr[(0...7)], r(arr)[(0...7)].run($c).to_a)
    assert_equal(arr[(0...-2)], r(arr)[(0...-2)].run($c).to_a)
    assert_equal(arr[(-2..-1)], r(arr)[(-2..-1)].run($c).to_a)
    assert_equal(arr[(0..-1)], r(arr)[(0..-1)].run($c).to_a)

    assert_equal(3, r(arr)[3].run($c))
    assert_equal(9, r(arr)[-1].run($c))
    assert_raise(RuntimeError) {r(0)[0].run($c)}
    assert_raise(ArgumentError) {r(arr)[0.1].run($c)}
    assert_raise(RuntimeError) {r([0])[1].run($c)}

    assert_equal(0, r([]).count.run($c))
    assert_equal(arr.length, r(arr).count.run($c))
    assert_raise(RuntimeError) {r(0).count.run($c)}
  end

  # from python tests
  def test_stream_fancy
    assert_equal([], r.limit([], 0).run($c))
    assert_equal([], r.limit([1, 2], 0).run($c))
    assert_equal([1], r.limit([1, 2], 1).run($c))
    assert_equal([1, 2], r.limit([1, 2], 5).run($c))
    assert_raise(RuntimeError) {r.limit([], -1).run($c)}

    assert_equal([], r.skip([], 0).run($c))
    assert_equal([], r.skip([1, 2], 5).run($c))
    assert_equal([1, 2], r.skip([1, 2], 0).run($c))
    assert_equal([2], r.skip([1, 2], 1).run($c))

    assert_equal([], r.distinct([]).run($c))
    assert_equal([1, 2, 3], r.distinct(([1, 2, 3] * 10)).run($c))
    assert_equal([1, 2, 3], r.distinct([1, 2, 3, 2]).run($c))
    assert_equal([false, true, 2.0], r.distinct([true, 2, false, 2]).run($c))
  end

  def test_ordering
    docs = (0..9).map {|n| {"id" => ((100 + n)), "a" => (n), "b" => ((n % 3))}}
    assert_equal(docs.sort_by {|x| x["a"]}, r(docs).orderby(:s).run($c))
    assert_equal(docs.sort_by {|x| x["a"]}.reverse, r(docs).orderby('-a').run($c))
    assert_equal(docs.select {|x| (x["b"] == 0)}.sort_by {|x| x["a"]},
                 r(docs).filter{|x| x[:b].eq 0}.orderby('+a').run($c))
    assert_equal(docs.select {|x| (x["b"] == 0)}.sort_by {|x| x["a"]}.reverse,
                 r(docs).filter{|x| x[:b].eq 0}.orderby('-a').run($c))
  end

  # +,-,%,*,/,<,>,<=,>=,eq,ne,any,all
  def test_ops
    assert_equal(8, (r(5) + 3).run($c))
    assert_equal(8, r(5).add(3).run($c))
    assert_equal(8, r.add(5, 3).run($c))
    assert_equal(8, r.add(2, 3, 3).run($c))

    assert_equal(2, (r(5) - 3).run($c))
    assert_equal(2, r(5).sub(3).run($c))
    assert_equal(2, r.sub(5, 3).run($c))

    assert_equal(2, (r(5) % 3).run($c))
    assert_equal(2, r(5).mod(3).run($c))
    assert_equal(2, r.mod(5, 3).run($c))

    assert_equal(15, (r(5) * 3).run($c))
    assert_equal(15, r(5).mul(3).run($c))
    assert_equal(15, r.mul(5, 3).run($c))

    assert_equal(5, (r(15) / 3).run($c))
    assert_equal(5, r(15).div(3).run($c))
    assert_equal(5, r.div(15, 3).run($c))

    assert_equal(false, r.lt(3, 2).run($c))
    assert_equal(false, r.lt(3, 3).run($c))
    assert_equal(true, r.lt(3, 4).run($c))
    assert_equal(false, r(3).lt(2).run($c))
    assert_equal(false, r(3).lt(3).run($c))
    assert_equal(true, r(3).lt(4).run($c))
    assert_equal(false, (r(3) < 2).run($c))
    assert_equal(false, (r(3) < 3).run($c))
    assert_equal(true, (r(3) < 4).run($c))

    assert_equal(false, r.le(3, 2).run($c))
    assert_equal(true, r.le(3, 3).run($c))
    assert_equal(true, r.le(3, 4).run($c))
    assert_equal(false, r(3).le(2).run($c))
    assert_equal(true, r(3).le(3).run($c))
    assert_equal(true, r(3).le(4).run($c))
    assert_equal(false, (r(3) <= 2).run($c))
    assert_equal(true, (r(3) <= 3).run($c))
    assert_equal(true, (r(3) <= 4).run($c))

    assert_equal(true, r.gt(3, 2).run($c))
    assert_equal(false, r.gt(3, 3).run($c))
    assert_equal(false, r.gt(3, 4).run($c))
    assert_equal(true, r(3).gt(2).run($c))
    assert_equal(false, r(3).gt(3).run($c))
    assert_equal(false, r(3).gt(4).run($c))
    assert_equal(true, (r(3) > 2).run($c))
    assert_equal(false, (r(3) > 3).run($c))
    assert_equal(false, (r(3) > 4).run($c))

    assert_equal(true, r.ge(3, 2).run($c))
    assert_equal(true, r.ge(3, 3).run($c))
    assert_equal(false, r.ge(3, 4).run($c))
    assert_equal(true, r(3).ge(2).run($c))
    assert_equal(true, r(3).ge(3).run($c))
    assert_equal(false, r(3).ge(4).run($c))
    assert_equal(true, (r(3) >= 2).run($c))
    assert_equal(true, (r(3) >= 3).run($c))
    assert_equal(false, (r(3) >= 4).run($c))

    assert_equal(false, r.eq(3, 2).run($c))
    assert_equal(true, r.eq(3, 3).run($c))

    assert_equal(true, r.ne(3, 2).run($c))
    assert_equal(false, r.ne(3, 3).run($c))

    assert_equal(true, r.all(true, true, true).run($c))
    assert_equal(false, r.all(true, false, true).run($c))
    assert_equal(true, r.and(true, true, true).run($c))
    assert_equal(false, r.and(true, false, true).run($c))
    assert_equal(true, r(true).and(true).run($c))
    assert_equal(false, r(true).and(false).run($c))

    assert_equal(false, r.any(false, false, false).run($c))
    assert_equal(true, r.any(false, true, false).run($c))

    assert_equal(false, r.or(false, false, false).run($c))
    assert_equal(true, r.or(false, true, false).run($c))
    assert_equal(false, r(false).or(false).run($c))
    assert_equal(true, r(true).or(false).run($c))
  end

  # TABLE
  def test_easy_read
    assert_equal(id_sort(tbl.run($c).to_a), data)
    assert_equal(id_sort(r.db("test").table("tbl").run($c).to_a), data)
  end

  # IF, JSON, ERROR
  def test_error
    query = r.branch((r.add(1, 2) >= 3), r([1,2,3]), r.error("unreachable"))
    query_err = r.branch((r.add(1, 2) > 3), r([1,2,3]), r.error("reachable"))
    assert_equal([1, 2, 3], query.run($c))
    assert_raise(RuntimeError) {assert_equal(query_err.run($c))}
  end

  # BOOL, JSON_NULL, ARRAY, ARRAYTOSTREAM
  def test_array
    assert_equal([true, false, nil], r([true, false, nil]).run($c))
    assert_equal([true, false, nil],
                 r([true, false, nil]).coerce_to("array").run($c).to_a)
  end

  # OBJECT, GETBYKEY
  def test_getbykey
    query = r("obj" => (tbl.get(0)))
    query2 = r("obj" => (tbl.get(0)))
    assert_equal(data[0], query.run($c)["obj"])
    assert_equal(data[0], query2.run($c)["obj"])
  end

  # MAP, FILTER, GETATTR, IMPLICIT_GETATTR, STREAMTOARRAY
  def test_map
    assert_equal([1, 2], r([{:id => 1}, {:id => 2}]).map {|row| row[:id]}.run($c).to_a)
    assert_raise(RuntimeError) {r(1).map {}.run($c).to_a}
    assert_equal([data[1]], tbl.filter("num" => 1).run($c).to_a)
    query = tbl.order_by(:id).map do |outer_row|
      tbl.filter {|row| (row[:id] < outer_row[:id])}.coerce_to("array")
    end
    assert_equal(data[(0..1)], id_sort(query.run($c).to_a[2]))
  end

  # REDUCE, HASATTR, IMPLICIT_HASATTR
  def test_reduce
    # TODO: Error checking for reduce
    assert_equal(6, r([1, 2, 3]).reduce(0) {|a, b| (a + b)}.run($c))
    assert_raise(RuntimeError) {r(1).reduce(0) {0}.run($c)}

    # assert_equal(  tbl.map{|row| row['id']}.reduce(0){|a,b| a+b}.run($c),
    #              data.map{|row| row['id']}.reduce(0){|a,b| a+b})

    assert_equal(data.map{true}, tbl.map {|r| r.contains(:id)}.run($c).to_a)
    assert_equal(data.map{false},
                 tbl.map {|row| r.not(row.contains("id"))}.run($c).to_a)
    assert_equal(data.map{false},
                 tbl.map {|row| r.not(row.contains("id"))}.run($c).to_a)
    assert_equal(data.map{false},
                 tbl.map {|row| r.not(row.contains("id"))}.run($c).to_a)
    assert_equal(data.map{true}, tbl.map {|r| r.contains(:id)}.run($c).to_a)
    assert_equal(data.map{false}, tbl.map {|r| r.contains("id").not}.run($c).to_a)
  end

  # assert_equal(  tbl.map{|row| row['id']}.reduce(0){|a,b| a+b}.run($c),
  #              data.map{|row| row['id']}.reduce(0){|a,b| a+b})
  # FILTER
  def test_filter
    assert_equal([1, 2], r([1, 2, 3]).filter {|x| (x < 3)}.run($c).to_a)
    assert_raise(RuntimeError) {r(1).filter {true}.run($c).to_a}
    query_5 = tbl.filter {|r| r[:name].eq("5")}
    assert_equal([data[5]], query_5.run($c).to_a)
    query_2345 = tbl.filter {|row| r.and((row[:id] >= 2), (row[:id] <= 5))}
    query_234 = query_2345.filter {|row| row[:num].ne(5)}
    query_23 = query_234.filter {|row| r.any(row[:num].eq(2), row[:num].eq(3))}
    assert_equal(data[(2..5)], id_sort(query_2345.run($c).to_a))
    assert_equal(data[(2..4)], id_sort(query_234.run($c).to_a))
    assert_equal(data[(2..3)], id_sort(query_23.run($c).to_a))
    assert_equal(data[(0...5)], id_sort(tbl.filter {|r| (r[:id] < 5)}.run($c).to_a))
  end

  def test_joins
    db_name = rand((2 ** 64)).to_s
    tbl_name = rand((2 ** 64)).to_s
    r.db_create(db_name).run($c)
    r.db(db_name).table_create((tbl_name + "1")).run($c)
    r.db(db_name).table_create((tbl_name + "2")).run($c)
    tbl1 = r.db(db_name).table((tbl_name + "1"))
    tbl2 = r.db(db_name).table((tbl_name + "2"))

    assert_equal({"inserted" => 10},
                 tbl1.insert((0...10).map {|x| {:id => (x), :a => ((x * 2))}}).run($c))
    assert_equal({"inserted" => 10},
                 tbl2.insert((0...10).map {|x|
                               {:id => (x), :b => ((x * 100))}
                             }).run($c))

    tbl1.eq_join(:a, tbl2).run($c).each do |pair|
      assert_equal(pair["right"]["id"], pair["left"]["a"])
    end

    assert_equal(tbl1.inner_join(tbl2){|l, r|
                   l[:a].eq(r[:id])}.run($c).to_a.sort_by do |x|
                   x["left"]["id"]
                 end,
                 tbl1.eq_join(:a, tbl2).run($c).to_a.sort_by {|x| x["left"]["id"]})

    assert_equal((tbl1.eq_join(:a, tbl2).run($c).to_a +
                  tbl1.filter{|row| tbl2.get(row[:a]).eq(nil)}.map {|row|
      {:left => row}}.run($c).to_a).sort_by {|x| x['left']['id']},
        tbl1.outer_join(tbl2) {|l,r|
        l[:a].eq(r[:id])}.run($c).to_a.sort_by {|x| x['left']['id']})

    assert_equal(tbl1.outer_join(tbl2) {
                   |l,r| l[:a].eq(r[:id])}.zip.run($c).to_a.sort_by {|x| x['a']},
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
    r.db_drop(db_name).run($c)
  end

  def test_random_insert_regressions
    assert_raise(RuntimeError){tbl.insert(true).run($c)}
    assert_not_nil(tbl.insert([true, true]).run($c)['errors'])
  end

  def test_too_big_key
    assert_not_nil(tbl.insert({:id => (("a" * 1000))}).run($c)["first_error"])
  end

  def test_key_generation
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    res = tbl.insert([[], {}, {:a => 1}, {:id => -1, :a => 2}]).run($c)
    assert_equal(1, res["errors"])
    assert_equal(3, res["inserted"])
    assert_equal(2, res["generated_keys"].length)
    assert_equal(1, tbl.get(res["generated_keys"][0]).delete.run($c)["deleted"])
    assert_equal(2,
                 tbl.filter{|x| (x[:id] < 0)|(x[:id] > 1000)}.delete.run($c)["deleted"])
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
  end

  # SLICE
  def test_slice
    arr = [0, 1, 2, 3, 4, 5]
    assert_equal(1, r(arr)[1].run($c))
    assert_equal(r(arr)[(2..-1)].run($c).to_a, r(arr)[(2...6)].run($c).to_a)
    assert_equal(r(arr)[(2..4)].run($c).to_a, r(arr)[(2...5)].run($c).to_a)
    assert_raise(ArgumentError) {r(arr)[(2...0)].run($c).to_a}
  end

  def test_mapmerge
    assert_equal({"a" => 1, "b" => 2}, r(:a => 1).merge(:b => 2).run($c))
    assert_equal({"a" => 2}, r.merge({:a => 1}, :a => 2).run($c))
  end

  # ORDERBY, MAP
  def test_order_by
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_equal(data, tbl.order_by("id").run($c).to_a)
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_equal(data, tbl.order_by(:"+id").run($c).to_a)
    assert_equal(data.reverse, tbl.order_by(:"-id").run($c).to_a)
    query = tbl.map {|x|
      r(:id => (x[:id]), :num => (x[:id].mod(2)))
   }.order_by(:num, '-id')
    want = data.map {|o| o["id"]}.sort_by {|n| (((n % 2) * data.length) - n)}
    assert_equal(want, query.run($c).to_a.map {|o| o["id"]})
  end

  # CONCATMAP, DISTINCT
  def test_concatmap
    assert_equal([1, 2, 1, 2, 1, 2], r([1, 2, 3]).concat_map {r([1, 2])}.run($c).to_a)
    assert_equal([1, 2], r([[1], [2]]).concat_map {|x| x}.run($c).to_a)
    assert_raise(RuntimeError) {r([[1], 2]).concat_map {|x| x}.run($c).to_a}
    assert_raise(RuntimeError) {r(1).concat_map {|x| x}.run($c).to_a}
    assert_equal([1, 2], r([[1], [2]]).concat_map {|x| x}.run($c).to_a)
    query = tbl.concat_map {|row| tbl.map {|row2| (row2[:id] * row[:id])}}.distinct
    nums = data.map {|o| o["id"]}
    want = nums.map {|n| nums.map {|m| (n * m)}}.flatten(1).uniq
    assert_equal(want.sort, query.run($c).to_a.sort)
  end

  # RANGE
  def test_range
#    assert_raise(RuntimeError) {tbl.between(1, r([3])).run($c).to_a}
#    assert_raise(RuntimeError) {tbl.between(2, 1).run($c).to_a}
    assert_equal(data[(1..3)], id_sort(tbl.between(1, 3).run($c).to_a))
    assert_equal(data[(3..3)], id_sort(tbl.between(3, 3).run($c).to_a))
    assert_equal(data[(2..-1)], id_sort(tbl.between(2, nil).run($c).to_a))
    assert_equal(data[(1..3)], id_sort(tbl.between(1, 3).run($c).to_a))
    assert_equal(data[(0..4)], id_sort(tbl.between(nil, 4).run($c).to_a))

    assert_raise(RuntimeError) {r([1]).between(1, 3).run($c).to_a}
    assert_raise(RuntimeError) {r([1, 2]).between(1, 3).run($c).to_a}
  end


  # GROUPEDMAPREDUCE
  def test_groupedmapreduce
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_equal(data, tbl.coerce_to("array").order_by(:id).run($c).to_a)
    assert_equal([0, 1],
                 r([{:id => 1},{:id => 0}]).order_by(:id).run($c).to_a.map{|x| x["id"]})
    assert_raise(RuntimeError) {r(1).order_by(:id).run($c).to_a}
    assert_raise(RuntimeError) {r([1]).nth(0).order_by(:id).run($c).to_a}
    assert_equal([1], r([1]).order_by(:id).run($c).to_a)
    assert_equal([{'num' => 1}], r([{:num => 1}]).order_by(:id).run($c).to_a)
    assert_equal([], r([]).order_by(:id).run($c).to_a)

    gmr = tbl.grouped_map_reduce(lambda {|row| (row[:id] % 4)},
                                 lambda {|row| row[:id]}, 0, lambda {|a, b| (a + b)})
    gmr2 = tbl.grouped_map_reduce(lambda {|r| (r[:id] % 4)}, lambda {|r| r[:id]},
                                  0, lambda {|a, b| (a + b)})
    gmr3 = tbl.coerce_to("array").grouped_map_reduce(lambda {|row| (row[:id] % 4)},
                                                  lambda {|row| row[:id]},
                                                  0, lambda {|a, b| (a + b)})
    gmr4 = r(data).grouped_map_reduce(lambda {|row| (row[:id] % 4)},
                                      lambda {|row| row[:id]},
                                      0, lambda {|a, b| (a + b)})
    assert_equal(gmr2.run($c).to_a, gmr.run($c).to_a)
    assert_equal(gmr3.run($c).to_a, gmr.run($c).to_a)
    assert_equal(gmr4.run($c).to_a, gmr.run($c).to_a)
    gmr5 = r([data]).grouped_map_reduce(lambda {|row| (row[:id] % 4)},
                                        lambda {|row| row[:id]},
                                        0, lambda {|a, b| (a + b)})
    gmr6 = r(1).grouped_map_reduce(lambda {|row| (row[:id] % 4)},
                                   lambda {|row| row[:id]}, 0, lambda {|a, b| (a + b)})
    assert_raise(RuntimeError) {gmr5.run($c).to_a}
    assert_raise(RuntimeError) {gmr6.run($c).to_a}
    gmr.run($c).to_a.each do |obj|
      want = data.map {|x| x["id"]}.select {|x| ((x % 4) == obj["group"])}.reduce(0, :+)
      assert_equal(want, obj["reduction"])
    end
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
  end

  # NTH
  def test_nth
    assert_equal(data[2], tbl.order_by(:id).nth(2).run($c))
    assert_equal(data[-1], tbl.order_by(:id).nth(-1).run($c))
    assert_not_nil(tbl.nth(-1).run($c))
  end

  def test_pluck
    e = r([{"a" => 1, "b" => 2, "c" => 3}, {"a" => 4, "b" => 5, "c" => 6}])
    assert_equal([{}, {}], e.pluck.run($c))
    assert_equal([{"a" => 1}, {"a" => 4}], e.pluck("a").run($c))
    assert_equal([{"a" => 1, "b" => 2}, {"a" => 4, "b" => 5}],
                 e.pluck("a", "b").run($c))
  end

  # PICKATTRS, # UNION, # LENGTH
  def test_pickattrs
    q1 = r.union(tbl.map{|r| r.pluck(:id, :name)}, tbl.map{|row| row.pluck(:id, :num)})
    q2 = r.union(tbl.map{|r| r.pluck(:id, :name)}, tbl.map {|row| row.pluck(:id, :num)})
    q1v = q1.run($c).to_a.sort_by {|x| ((x["name"].to_s + ",") + x["id"].to_s)}
    assert_equal(q2.run($c).to_a.sort_by{|x| ((x["name"].to_s + ",") + x["id"].to_s)},
                 q1v)

    len = data.length

    assert_equal((2 * len), q2.count.run($c))
    assert_equal(Array.new(len, nil), q1v.map {|o| o["num"]}[(len..((2 * len) - 1))])
    assert_equal(Array.new(len, nil), q1v.map {|o| o["name"]}[(0..(len - 1))])
  end

  def test_pointdelete
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_equal({"deleted" => 1}, tbl.get(0).delete.run($c))
    assert_equal({"skipped" => 1}, tbl.get(0).delete.run($c))
    assert_equal(data[(1..-1)], tbl.order_by(:id).run($c).to_a)
    assert_equal({"inserted" => 1}, tbl.insert(data[0]).run($c))
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
  end

  def test_write_python # from python tests
    tbl.delete.run($c)
    docs = [{"a" => 3, "b" => 10, "id" => 1},
            {"a" => 9, "b" => -5, "id" => 2},
             {"a" => 9, "b" => 3, "id" => 3}]
    assert_equal({"inserted" => (docs.length)}, tbl.insert(docs).run($c))
    docs.each {|doc| assert_equal(doc, tbl.get(doc["id"]).run($c))}
    assert_equal([docs[0]], tbl.filter("a" => 3).run($c).to_a)
    assert_raise(RuntimeError) {tbl.filter("a" => ((tbl.count + ""))).run($c).to_a}
    assert_equal(nil, tbl.get(0).run($c))
    assert_equal({"inserted" => 1},
                 tbl.insert({:id => 100, :text => "\u{30b0}\u{30eb}\u{30e1}"}).run($c))
    assert_equal("\u{30b0}\u{30eb}\u{30e1}", tbl.get(100)[:text].run($c))
    tbl.delete.run($c)
    docs = [{"id" => 1}, {"id" => 2}]
    assert_equal({"inserted" => (docs.length)}, tbl.insert(docs).run($c))
    assert_equal({"deleted" => 1}, tbl.limit(1).delete.run($c))
    assert_equal(1, tbl.run($c).to_a.length)
    tbl.delete.run($c)
    docs = (0...4).map {|n| {"id" => ((100 + n)), "a" => (n), "b" => ((n % 3))}}
    assert_equal({"inserted" => (docs.length)}, tbl.insert(docs).run($c))
    docs.each {|doc| assert_equal(doc, tbl.get(doc["id"]).run($c))}
    tbl.delete.run($c)
    docs = (0...10).map {|n| {"id" => ((100 + n)), "a" => (n), "b" => ((n % 3))}}
    assert_equal({"inserted" => (docs.length)}, tbl.insert(docs).run($c))

    # TODO: still an error?
    # assert_raise(ArgumentError){r[:a] == 5}
    # assert_raise(ArgumentError){r[:a] != 5}
    assert_equal(docs.select {|x| (x["a"] < 5)},
                 id_sort(tbl.filter {|r| (r[:a] < 5)}.run($c).to_a))
    assert_equal(docs.select {|x| (x["a"] <= 5)},
                 id_sort(tbl.filter {|r| (r[:a] <= 5)}.run($c).to_a))
    assert_equal(docs.select {|x| (x["a"] > 5)},
                 id_sort(tbl.filter {|r| (r[:a] > 5)}.run($c).to_a))
    assert_equal(docs.select {|x| (x["a"] >= 5)},
                 id_sort(tbl.filter {|r| (r[:a] >= 5)}.run($c).to_a))
    assert_equal(docs.select {|x| (x["a"] == x["b"])},
                 id_sort(tbl.filter {|r| r[:a].eq(r[:b])}.run($c).to_a))

    assert_equal(r(-5).run($c), -r(5).run($c))
    assert_equal(r(5).run($c), +r(5).run($c))

    assert_equal(7, (r(3) + 4).run($c))
    assert_equal(-1, (r(3) - 4).run($c))
    assert_equal(12, (r(3) * 4).run($c))
    assert_equal((3.0 / 4), (r(3) / 4).run($c))
    assert_equal((3 % 2), (r(3) % 2).run($c))
    assert_equal(7, (4 + r(3)).run($c))
    assert_equal(1, (4 - r(3)).run($c))
    assert_equal(12, (4 * r(3)).run($c))
    assert_equal(1, (4 / r(4)).run($c))
    assert_equal(1, (4 % r(3)).run($c))

    assert_equal(84, (((r(3) + 4) * -r(6)) * (r(-5) + 3)).run($c))

    tbl.delete.run($c)
    docs = (0...10).map {|n| {"id" => ((100 + n)), "a" => (n), "b" => ((n % 3))}}
    assert_equal({"inserted" => (docs.length)}, tbl.insert(docs).run($c))
    assert_equal({"unchanged" => (docs.length)}, tbl.replace {|x| x}.run($c))
    assert_equal(docs, id_sort(tbl.run($c).to_a))
    assert_equal({"unchanged" => 10}, tbl.update {nil}.run($c))

    res = tbl.update {1}.run($c)
    assert_equal(10, res["errors"])
    res = tbl.update {|row| row[:id]}.run($c)
    assert_equal(10, res["errors"])
    res = tbl.update {|r| r[:id]}.run($c)
    assert_equal(10, res["errors"])

    res = tbl.update {|row| r.branch((row[:id] % 2).eq(0), {:id => -1}, row)}.run($c)
    assert_not_nil(res["first_error"])
    filtered_res = res.reject {|k, v| (k == "first_error")}
    assert_not_equal(filtered_res, res)
    assert_equal({"unchanged" => 5, "errors" => 5}, filtered_res)

    assert_equal(docs, id_sort(tbl.run($c).to_a))
  end

  def test_getitem
    arr = (0...10).map {|x| x}
    [(0..-1), (2..-1), (0...2), (-1..-1), (0...-1), (3...5), 3, -1].each do |rng|
      assert_equal(arr[rng], r(arr)[rng].run($c))
    end
    obj = {:a => 3, :b => 4}
    [:a, :b].each {|attr| assert_equal(obj[attr], r(obj)[attr].run($c))}
    assert_raise(RuntimeError) {r(obj)[:c].run($c)}
  end

  def test_contains
    obj = r(:a => 1, :b => 2)
    assert_equal(true, obj.contains(:a).run($c))
    assert_equal(false, obj.contains(:c).run($c))
    assert_equal(true, obj.contains(:a, :b).run($c))
    assert_equal(false, obj.contains(:a, :c).run($c))
  end

  #FOREACH
  def test_array_foreach
    assert_equal(data, id_sort(tbl.run($c).to_a))
    assert_equal({"unchanged" => 1, "replaced" => 5}, r([2, 3, 4]).for_each do |x|
      [tbl.get(x).update {{:num => 0}}, tbl.get((x * 2)).update {{:num => 0}}]
    end.run($c))
    assert_equal({"unchanged" => 5, "replaced" => 5},
                 tbl.coerce_to("array").for_each do |row|
      tbl.filter {|r| r[:id].eq(row[:id])}.update do |row|
        r.branch(row[:num].eq(0), {:num => (row[:id])}, nil)
      end
    end.run($c))
    assert_equal(data, id_sort(tbl.run($c).to_a))
  end

  #FOREACH
  def test_foreach_multi
    db_name = ("foreach_multi" + rand((2 ** 64)).to_s)
    table_name = ("foreach_multi_tbl" + rand((2 ** 64)).to_s)
    table_name_2 = (table_name + rand((2 ** 64)).to_s)
    table_name_3 = (table_name_2 + rand((2 ** 64)).to_s)
    table_name_4 = (table_name_3 + rand((2 ** 64)).to_s)
    assert_equal({"created"=>1.0}, r.db_create(db_name).run($c))
    assert_equal({"created"=>1.0}, r.db(db_name).table_create(table_name).run($c))
    assert_equal({"created"=>1.0}, r.db(db_name).table_create(table_name_2).run($c))
    assert_equal({"created"=>1.0}, r.db(db_name).table_create(table_name_3).run($c))
    assert_equal({"created"=>1.0}, r.db(db_name).table_create(table_name_4).run($c))
    tbl = r.db(db_name).table(table_name)
    tbl2 = r.db(db_name).table(table_name_2)
    tbl3 = r.db(db_name).table(table_name_3)
    tbl4 = r.db(db_name).table(table_name_4)
    assert_equal({"inserted" => 10}, tbl.insert(data).run($c))
    assert_equal({"inserted" => 10}, tbl2.insert(data).run($c))
    query = tbl.for_each do |row|
      tbl2.for_each do |row2|
        tbl3.insert({"id" => (((row[:id] * 1000) + row2[:id]))})
      end
    end
    assert_equal({"inserted" => 100}, query.run($c))
    query = tbl.for_each do |row|
      tbl2.filter {|r| r[:id].eq((row[:id] * row[:id]))}.delete
    end
    assert_equal({"deleted" => 4}, query.run($c))
    query = tbl.for_each do |row|
      tbl2.for_each do |row2|
        tbl3.filter {|r| r[:id].eq(((row[:id] * 1000) + row2[:id]))}.delete
      end
    end
    assert_equal({"deleted" => 60}, query.run($c))
    assert_equal({"inserted" => 10}, tbl.for_each {|row| tbl4.insert(row)}.run($c))
    assert_equal({"deleted" => 6}, tbl2.for_each {|row|
                   tbl4.filter {|r| r[:id].eq(row[:id])}.delete
                }.run($c))

    # TODO: Pending resolution of issue #930
    # assert_equal(tbl3.order_by(:id).run($c),
    #              r.let([['x', tbl2.concat_map{|row|
    #                        tbl.filter{
    #                          r[:id].neq(row[:id])
    #                       }
    #                     }.map{r[:id]}.distinct.coerce_to("array")]],
    #                    tbl.concat_map{|row|
    #                      r[:$x].to_stream.map{|id| row[:id]*1000+id}}).run($c))
  ensure
    r.db_drop(db_name).run($c)
  end

  #FOREACH
  def test_fancy_foreach
    db_name = rand((2 ** 64)).to_s
    table_name = rand((2 ** 64)).to_s

    assert_equal({"created"=>1.0}, r.db_create(db_name).run($c))
    assert_equal({"created"=>1.0}, r.db(db_name).table_create(table_name).run($c))
    tbl = r.db(db_name).table(table_name)
    assert_equal({"inserted" => 10}, tbl.insert(data).run($c))
    tbl.insert({:id => 11, :broken => (true)}).run($c)
    tbl.insert({:id => 12, :broken => (true)}).run($c)
    tbl.insert({:id => 13, :broken => (true)}).run($c)

    query = tbl.order_by(:id).for_each do |row|
      [tbl.update {|row2| r.branch(row[:id].eq(row2[:id]),
                                    {:num => ((row2[:num] * 2))}, nil)},
        tbl.filter {|r| r[:id].eq(row[:id])}.replace {|row|
         r.branch((row[:id] % 2).eq(1), nil, row)},
        tbl.get(0).delete, tbl.get(12).delete]
    end

    res = query.run($c)

    assert_equal(2, res["errors"])
    assert_not_nil(res["first_error"])
    assert_equal(9, res["deleted"])
    assert_equal(102, res["unchanged"])
    assert_equal(9, res["replaced"])
    assert_equal([{"num" => 4, "id" => 2, "name" => "2"},
                  {"num" => 8, "id" => 4, "name" => "4"},
                  {"num" => 12, "id" => 6, "name" => "6"},
                  {"num" => 16, "id" => 8, "name" => "8"}],
                 tbl.order_by(:id).run($c).to_a)

  ensure
    r.db_drop(db_name).run($c)
  end

  def test_bad_primary_key_type
    assert_not_nil(tbl.insert({:id => ([])}).run($c)["first_error"])
    assert_not_nil(tbl.get(100).replace{{:id => ([])}}.run($c)["first_error"])
  end

  def test_big_between
    data_100 = (0..99).map {|x| {:id => (x)}}
    res = tbl.insert(data_100).run($c)
    assert_equal(10, res['errors'])
    assert_equal(90, res['inserted'])
    assert_equal([{"num"=>1.0, "name"=>"1", "id"=>1.0},
                  {"num"=>2.0, "name"=>"2", "id"=>2.0}],
                 id_sort(tbl.between(1, 2).run($c).to_a))
    assert_equal({"deleted" => 100}, tbl.delete.run($c))
    assert_equal({"inserted" => 10}, tbl.insert(data).run($c))
    assert_equal(data, id_sort(tbl.run($c).to_a))
  end

  #POINTMUTATE
  def test_mutate_edge_cases
    res = tbl.replace {1}.run($c)
    assert_equal(10, res["errors"])
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_equal({"deleted" => 1}, tbl.get(0).replace {nil}.run($c))
    assert_equal(data[(1..-1)], tbl.order_by(:id).run($c).to_a)
    assert_equal({"skipped" => 1}, tbl.get(0).replace {nil}.run($c))
    assert_equal(data[(1..-1)], tbl.order_by(:id).run($c).to_a)
    assert_equal({"inserted" => 1}, tbl.get(0).replace {data[0]}.run($c))
    assert_not_nil(tbl.get(-1).replace {{:id => ([])}}.run($c)['first_error'])
    # TODO: maybe we *do* want this contraint?
    # assert_not_nil(tbl.get(-1).replace {{:id => 0}}.run($c)['first_error'])
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_not_nil(tbl.get(0).replace {|row|
                     r.branch(row.eq(nil), data[0], data[1])
                  }.run($c)['first_error'])
    assert_equal({"unchanged" => 1},
                 tbl.get(0).replace{|row|
                   r.branch(row.eq(nil), data[1], data[0])
                 }.run($c))
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
  end

  def test_insert
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    data2 = (data + data.map {|x| {"id" => ((x["id"] + data.length))}})

    res = tbl.insert(data2).run($c)
    assert_equal(data.length, res["inserted"])
    assert_equal(data.length, res["errors"])
    assert_not_nil(res["first_error"])

    assert_equal(data2, tbl.order_by(:id).run($c).to_a)
    assert_equal({"deleted" => (data2.length)}, tbl.delete.run($c))

    res = tbl.insert(data).run($c)
    assert_equal(data.length, res["inserted"])
    assert_equal(nil, res["errors"])
    assert_nil(res["first_error"])

    assert_equal(data, tbl.order_by(:id).run($c).to_a)
  end

  #POINTUPDATE
  def test_update_edge_cases
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    assert_equal({"unchanged" => 1}, tbl.get(0).update {nil}.run($c))
    assert_equal({"skipped" => 1}, tbl.get(11).update {nil}.run($c))
    assert_equal({"skipped" => 1}, tbl.get(11).update {{}}.run($c))
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
  end

  #FOREACH
  def test_foreach_error
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
    query = tbl.for_each {|row| tbl.get(row[:id]).update {{:id => 0}}}
    res = query.run($c)
    assert_equal(9, res["errors"])
    assert_not_nil(res["first_error"])
    assert_equal(data, tbl.order_by(:id).run($c).to_a)
  end

  def test_write
    db_name = rand((2 ** 64)).to_s
    table_name = rand((2 ** 64)).to_s
    orig_dbs = r.db_list.run($c)

    assert_equal({'created' => 1}, r.db_create(db_name).run($c))
    assert_raise(RuntimeError) {r.db_create(db_name).run($c)}

    new_dbs = r.db_list.run($c)

    assert_equal((orig_dbs.length + 1), new_dbs.length)
    assert_equal({'created' => 1}, r.db(db_name).table_create(table_name).run($c))
    assert_raise(RuntimeError) {r.db(db_name).table_create(table_name).run($c)}
    assert_equal([table_name], r.db(db_name).table_list.run($c))
    assert_equal({"inserted" => 1},
                 r.db(db_name).table(table_name).insert({:id => 0}).run($c))
    assert_equal([{"id" => 0}], r.db(db_name).table(table_name).run($c).to_a)
    assert_equal({'created' => 1},
                 r.db(db_name).table_create((table_name + table_name)).run($c))
    assert_equal(2, r.db(db_name).table_list.run($c).length)
    assert_raise(RuntimeError) {r.db("").table("").run($c).to_a}
    assert_raise(RuntimeError) {r.db("test").table(table_name).run($c).to_a}
    assert_raise(RuntimeError) {r.db(db_name).table("").run($c).to_a}
    assert_equal({'dropped' => 1},
                 r.db(db_name).table_drop((table_name + table_name)).run($c))
    assert_raise(RuntimeError) do
      r.db(db_name).table((table_name + table_name)).run($c).to_a
    end

    assert_equal({'dropped' => 1}, r.db_drop(db_name).run($c))
    assert_equal(orig_dbs.sort, r.db_list.run($c).sort)
    assert_raise(RuntimeError) {r.db(db_name).table(table_name).run($c).to_a}
    assert_equal({'created' => 1}, r.db_create(db_name).run($c))
    assert_raise(RuntimeError) {r.db(db_name).table(table_name).run($c).to_a}
    assert_equal({'created' => 1}, r.db(db_name).table_create(table_name).run($c))
    assert_equal([], r.db(db_name).table(table_name).run($c).to_a)

    tbl2 = r.db(db_name).table(table_name)
    len = data.length

    # insert, UPDATE, :upsert
    assert_equal(len, tbl2.insert(data, :upsert).run($c)["inserted"])
    assert_equal((len * 2), tbl2.insert((data + data), :upsert).run($c)["unchanged"])
    assert_equal(data, id_sort(tbl2.run($c).to_a))
    assert_equal(1, tbl2.insert({:id => 0, :broken => (true)},
                                :upsert).run($c)["replaced"])
    assert_equal(1, tbl2.insert({:id => 1, :broken => (true)},
                                :upsert).run($c)["replaced"])
    assert_equal(data[(2..(len - 1))], id_sort(tbl2.run($c).to_a)[(2..(len - 1))])
    assert_equal(true, id_sort(tbl2.run($c).to_a)[0]["broken"])
    assert_equal(true, id_sort(tbl2.run($c).to_a)[1]["broken"])
    mutate = tbl2.filter {|r| r[:id].eq(0)}.replace {data[0]}.run($c)
    assert_equal({"replaced" => 1}, mutate)
    update = tbl2.update {|x| r.branch(x.contains(:broken), data[1], x)}.run($c)
    assert_equal({"replaced" => 1, "unchanged" => 9}, update)
    mutate = tbl2.replace do |x|
      {:id => (x[:id]), :num => (x[:num]), :name => (x[:name])}
    end.run($c)
    assert_equal({"replaced" => 1, "unchanged" => 9}, mutate)
    assert_equal(data, id_sort(tbl2.run($c).to_a))
    mutate = tbl2.replace do |row|
      r.branch(row[:id].eq(4).|(row[:id].eq(5)), {:id => -1}, row)
    end.run($c)
    assert_not_nil(mutate["first_error"])
    filtered_mutate = mutate.reject {|k, v| (k == "first_error")}
    assert_equal({"unchanged" => 8, "errors" => 2}, filtered_mutate)
    assert_equal({"replaced" => 1}, tbl2.get(5).update {{:num => "5"}}.run($c))
    mutate = tbl2.replace {|row| r.merge(row, :num => ((row[:num] * 2)))}.run($c)
    assert_not_nil(mutate["first_error"])
    filtered_mutate = mutate.reject {|k, v| (k == "first_error")}
    assert_equal({"replaced" => 8, "unchanged" => 1, "errors" => 1}, filtered_mutate)
    assert_equal("5", tbl2.get(5).run($c)["num"])
    assert_equal({"replaced" => 9, "unchanged" => 1}, tbl2.replace do |row|
      r.branch(row[:id].eq(5), data[5], row.merge(:num => ((row[:num] / 2))))
    end.run($c))
    assert_equal(data, id_sort(tbl2.run($c).to_a))
    update = tbl2.get(0).update {{:num => 2}}.run($c)
    assert_equal({"replaced" => 1}, update)
    assert_equal(2, tbl2.get(0).run($c)["num"])
    update = tbl2.get(0).update {nil}.run($c)
    assert_equal({"unchanged" => 1}, update)
    assert_equal({"replaced" => 1}, tbl2.get(0).replace {data[0]}.run($c))
    assert_equal(data, id_sort(tbl2.run($c).to_a))

    # DELETE
    assert_equal(5, tbl2.filter {|r| (r[:id] < 5)}.delete.run($c)["deleted"])
    assert_equal(data[(5..-1)], id_sort(tbl2.run($c).to_a))
    assert_equal((len - 5), tbl2.delete.run($c)["deleted"])
    assert_equal([], tbl2.run($c).to_a)

    # insert
    assert_equal(len, tbl2.insert(r(data), :upsert).run($c)["inserted"])
    tbl2.insert(data, :upsert).run($c)

    assert_equal(data, id_sort(tbl2.run($c).to_a))

    # MUTATE
    assert_equal({"unchanged" => (len)}, tbl2.replace {|row| row}.run($c))
    assert_equal(data, id_sort(tbl2.run($c).to_a))
    assert_equal({"unchanged" => 5, "deleted" => ((len - 5))},
                 tbl2.replace {|row| r.branch((row[:id] < 5), nil, row)}.run($c))
    assert_equal(data[(5..-1)], id_sort(tbl2.run($c).to_a))
    assert_equal({"inserted" => 5}, tbl2.insert(data[(0...5)], :upsert).run($c))

    # FOREACH, POINTDELETE
    # TODO_SERVER: Return value of foreach should change (Issue #874)
    query = tbl2.for_each do |row|
      [tbl2.get(row[:id]).delete, tbl2.insert(row, :upsert)]
    end
    assert_equal({"deleted" => 10, "inserted" => 10}, query.run($c))
    assert_equal(data, id_sort(tbl2.run($c).to_a))
    tbl2.for_each {|row| tbl2.get(row[:id]).delete}.run($c)
    assert_equal([], id_sort(tbl2.run($c).to_a))

    tbl2.insert(data, :upsert).run($c)
    assert_equal(data, id_sort(tbl2.run($c)))
    query = tbl2.get(0).update {{:id => 0, :broken => 5}}
    assert_equal({"replaced" => 1}, query.run($c))
    query = tbl2.insert(data[0], :upsert)
    assert_equal({"replaced" => 1}, query.run($c))
    assert_equal(data, id_sort(tbl2.run($c).to_a))

    assert_equal({"deleted" => 1}, tbl2.get(0).replace {nil}.run($c))
    assert_equal(data[(1..-1)], id_sort(tbl2.run($c).to_a))
    assert_not_nil(tbl2.get(1).replace {{:id => -1}}.run($c)['first_error'])
    assert_not_nil(tbl2.get(1).replace {|row|
                     {:name => ((row[:name] * 2))}}.run($c)['first_error'])
    assert_equal({"replaced" => 1},
                 tbl2.get(1).replace {|row| row.merge(:num => 2)}.run($c))
    assert_equal(2, tbl2.get(1).run($c)["num"])
    assert_equal({"inserted" => 1, "replaced" => 1},
                 tbl2.insert(data[(0...2)], :upsert).run($c))

    assert_equal(data, id_sort(tbl2.run($c).to_a))
  ensure
    begin
      r.db_drop(db_name).run($c)
    rescue Exception => e
    end
  end

  def test_outdated_raise
    outdated = r.db("test").table("tbl", :use_outdated => (true))

    assert_raise(RuntimeError) { outdated.insert({ :id => 0 }, :upsert).run($c) }

    assert_raise(RuntimeError) do
      outdated.filter { |row| (row[:id] < 5) }.update { {} }.run($c)
    end

    assert_raise(RuntimeError) { outdated.update { {} }.run($c) }

    assert_raise(RuntimeError) do
      outdated.filter { |row| (row[:id] < 5) }.delete.run($c)
    end

    assert_raise(RuntimeError) do
      outdated.filter { |row| (row[:id] < 5) }.between(2, 3).replace { nil }.run($c)
    end
  end

  def test_op_raise
    # assert_raise(ArgumentError) do
    #   r.db("a").table_create("b").run($c)(:bad_opt => (true))
    # end

    assert_raise(RuntimeError) do
      r.db("a").table_create("b", :bad_opt => (true)).run($c)
    end

    assert_raise(RuntimeError) { r.db("a").table("b", :bad_opt => (true)).run($c) }
    assert_raise(RuntimeError) { r.db("a").run($c) }
  end

  def test_close_and_reconnect
    assert_nothing_raised { c.close }
    c.reconnect
  end

  def setup
    begin
      r.db_create('test').run($c)
    rescue Exception => e
    end
    begin
      r.db('test').table_create('tbl').run($c)
    rescue Exception => e
    end

    tbl.delete.run($c)
    tbl.insert(data).run($c)
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

class DetTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  def rdb; r.db('test').table('tbl'); end
  @@c = RethinkDB::Connection.new('localhost', $port_base + 28016)
  def c; @@c; end
  def server_data; rdb.order_by(:id).run($c).to_a; end

  def test__init
    begin
      r.db('test').table_create('tbl').run($c)
    rescue
    end
    $data = (0...10).map{|x| {'id' => x}}
    rdb.delete.run($c)
    rdb.insert($data).run($c)
  end

  def test_det
    #TODO: JS tests here
    assert_raise(RuntimeError) {
      rdb.update{|row| {:count => rdb.get(0).map{|x| 0}}}.run($c)
    }
    assert_equal({'replaced'=>10}, rdb.update{|row| {:count => 0}}.run($c))

    assert_raise(RuntimeError){rdb.replace{|row| rdb.get(row[:id])}.run($c)}

    assert_raise(RuntimeError) {
      rdb.update{{:count => rdb.map{|x| x[:count]}.reduce(0){|a,b| a+b}}}.run($c)
    }
    static = r.expr(server_data)
    assert_equal({'replaced'=>10},
                 rdb.update {
                   {:count => static.map{|x| x[:id]}.reduce(0){|a,b| a+b}}
                 }.run($c))
  end


  def test_nonatomic
    # UPDATE MODIFY
    assert_raise(RuntimeError){rdb.update{|row| {:x => rdb.get(0).do{1}}}.run($c)}
    assert_equal({"replaced" => 10},
                 rdb.update(:non_atomic){|row| {:x => rdb.get(0).do{1}}}.run($c))
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c), 10)

    assert_raise(RuntimeError){rdb.get(0).update{|row|{:x => rdb.get(0).do{1}}}.run($c)}
    assert_equal({"replaced" => 1},
                 rdb.get(0).update(:non_atomic){|row| {:x => rdb.get(0).do{2}}}.run($c))
    assert_equal(11, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    # UPDATE ERROR
    assert_not_nil(rdb.update{r.error("")}.run($c)['first_error'])
    assert_equal(11, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    #UPDATE SKIPPED
    assert_raise(RuntimeError){
      rdb.update{|row| r.branch(rdb.get(0).do{true}, nil, {:x => 0.1})}.run($c)
    }
    res = rdb.update(:non_atomic) { |row|
      r.branch(rdb.get(0).do{true}, nil, {:x => 0.1})
    }.run($c)
    assert_equal({"unchanged" => 10}, res)
    assert_equal(11, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    assert_raise(RuntimeError){
      rdb.get(0).update{r.branch(rdb.get(0).do{true}, nil, {:x => 0.1})}.run($c)
    }
    res = rdb.get(0).update(:non_atomic){
      r.branch(rdb.get(0).do{true}, nil, {:x => 0.1})
    }.run($c)
    assert_equal({"unchanged" => 1}, res)
    assert_equal(11, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    # MUTATE MODIFY
    assert_raise(RuntimeError) {
      rdb.get(0).replace{|row| r.branch(rdb.get(0).do{true}, row, nil)}.run($c)
    }
    res = rdb.get(0).replace(:non_atomic) {|row|
      r.branch(rdb.get(0).do{true},row,nil)
    }.run($c)
    assert_equal({"unchanged"=>1}, res)
    assert_equal(11, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    assert_raise(RuntimeError){rdb.replace{|row| rdb.get(0).do{row}}.run($c)}
    res = rdb.replace(:non_atomic) { |row|
      r.branch(rdb.get(0).do{row}[:id].eq(1), row.merge({:x => 2}), row)
    }.run($c)
    assert_equal({"unchanged"=>9.0, "replaced"=>1.0}, res)
    assert_equal(12, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    #MUTATE ERROR
    assert_raise(RuntimeError) {
      rdb.get(0).replace{|row| r.branch(rdb.get(0).do{r.error("")}, row, nil)}.run($c)
    }
    assert_not_nil(rdb.get(0).replace(:non_atomic) {|row|
                     r.branch(rdb.get(0).do{r.error("")},row,nil)
                   }.run($c)['first_error'])
    assert_equal(12, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    assert_raise(RuntimeError) {
      rdb.replace{|row| r.branch(rdb.get(0).do{r.error("")}, row, nil)}.run($c)
    }
    res = rdb.replace(:non_atomic) {|row|
      r.branch(rdb.get(0).do{r.error("")},row,nil)
    }.run($c)
    assert_equal(10, res['errors']); assert_not_nil(res['first_error'])
    assert_equal(12, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    #MUTATE DELETE
    assert_raise(RuntimeError){rdb.get(0).replace{|row|
        r.branch(rdb.get(0).do{true},nil,row)}.run($c)}
    res = rdb.get(0).replace(:non_atomic){|row|
      r.branch(rdb.get(0).do{true},nil,row)}.run($c)
    assert_equal({"deleted"=>1.0}, res)
    assert_equal(10, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    assert_raise(RuntimeError){
      rdb.replace{|row| r.branch(rdb.get(0).do{row}[:id] < 3, nil, row)}.run($c)
    }
    res = rdb.replace(:non_atomic) {|row|
      r.branch(rdb.get(0).do{row}[:id] < 3, nil, row)
    }.run($c)
    assert_equal({'deleted'=>2, 'unchanged'=>7}, res)
    assert_equal(7, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))

    #MUTATE INSERT
    assert_raise(RuntimeError){rdb.get(0).replace{
        {:id => 0, :count => rdb.get(3)[:count], :x => rdb.get(3)[:x]}}.run($c)}
    res = rdb.get(0).replace(:non_atomic){
      {:id => 0, :count => rdb.get(3)[:count], :x => rdb.get(3)[:x]}}.run($c)
    assert_equal({"inserted"=>1}, res)
    assert_raise(RuntimeError){rdb.get(1).replace{rdb.get(3).merge({:id => 1})}.run($c)}
    res = rdb.get(1).replace(:non_atomic){rdb.get(3).merge({:id => 1})}.run($c)
    assert_equal({"inserted"=>1}, res)
    res = rdb.get(2).replace(:non_atomic){rdb.get(1).merge({:id => 2})}.run($c)
    assert_equal({"inserted"=>1}, res)
    assert_equal(10, rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run($c))
  end

  def test_det_end
    assert_equal(rdb.map{|row| row[:count]}.reduce(0){|a,b| a+b}.run($c),
                 (rdb.map{|row| row[:id]}.reduce(0){|a,b| a+b} * rdb.count).run($c))
  end
end


