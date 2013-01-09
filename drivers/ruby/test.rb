# -*- coding: utf-8 -*-
# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift("./lib")
require './test_helper'
require './test_det'
require './test_bt'

class ClientTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  include RethinkDB::TestHelper

  def test_precedence_hacks
    lst = (0..9).to_a
    assert_raise(RuntimeError) { r(lst).filter { |x| ((x < r(3).|(x)) > r(5)) } }
  end

  def test_op_raise
    assert_raise(ArgumentError) do
      r.db("a").table_create("b").run(:bad_opt => (true))
    end

    assert_raise(ArgumentError) do
      r.db("a").table_create("b", :bad_opt => (true))
    end

    assert_raise(ArgumentError) { r.db("a").table("b", :bad_opt => (true)) }
  end

  def test_numops
    assert_raise(RuntimeError) { r.div(1, 0).run }
    assert_raise(RuntimeError) { r.div(0, 0).run }
    assert_raise(RuntimeError) { r.mul(1.0e+200, 1.0e+300).run }
    assert_equal(1.0e+200, r.mul(1.0e+100, 1.0e+100).run)
  end

  def test_outdated_raise
    outdated = r.db("test").table("tbl", :use_outdated => (true))

    assert_raise(RuntimeError) { outdated.insert({ :id => 0 }, :upsert) }

    assert_raise(RuntimeError) do
      outdated.filter { |row| (row[:id] < 5) }.update { {} }
    end

    assert_raise(RuntimeError) { outdated.update { {} } }

    assert_raise(RuntimeError) do
      outdated.filter { |row| (row[:id] < 5) }.delete
    end

    assert_raise(RuntimeError) do
      outdated.filter { |row| (row[:id] < 5) }.between(2, 3).replace { nil }
    end
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
    assert_equal({ "b" => 2 }, r(:a => 1, :b => 2, :c => 3).unpick(:a, :c).run)
    assert_equal({ "a" => 1 }, r(:a => 1).unpick(:b).run)
    assert_equal({}, r({}).unpick(:b).run)
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
    assert_equal([1, 1], r.let(:a => 1) { [r.letvar("a"), r.letvar("a")] }.run)
  end

  def test_let
    assert_equal(3, r.let([["x", 3]]) { r.letvar("x") }.run)
    assert_equal(4, r.let([["x", 3], ["x", 4]]) { r.letvar("x") }.run)
    assert_equal(3, r.let([["x", 3], ["y", 4]]) { r.letvar("x") }.run)
    assert_equal(6, r.let([["a", 2], ["b", (r.letvar("a") + 1)]]) { (r.letvar("b") * 2) }.run)
    assert_equal(3, r.let(:x => 3) { r.letvar("x") }.run)
    assert_equal(4, r.let(:x => 3, :y => 4) { r.letvar("y") }.run)
  end

  def test_if
    assert_equal(3, r.branch(true, 3, 4).run)
    assert_equal(5, r.branch(false, 4, 5).run)
    assert_equal("foo", r.branch(r.eq(3, 3), "foo", "bar").run)
    assert_raise(RuntimeError) { r.branch(5, 1, 2).run }
  end

  def test_attr # from python tests
    # TODO: Mimic object notation?
    # self.expect(I.Has({"foo": 3}, "foo"), True)
    # self.expect(I.Has({"foo": 3}, "bar"), False)
    # self.expect(I.Attr({"foo": 3}, "foo"), 3)
    # self.error_exec(I.Attr({"foo": 3}, "bar"), "missing")
    # self.expect(I.Attr(I.Attr({"a": {"b": 3}}, "a"), "b"), 3)
  end

  def test_array_python # from python tests
    assert_equal([2], r([]).append(2).run.to_a)
    assert_equal([1, 2], r([1]).append(2).run.to_a)
    assert_raise(RuntimeError) { r(2).append(0).run.to_a }

    assert_equal([1, 2], r.add([1], [2]).run.to_a)
    assert_equal([1, 2], r.add([1, 2], []).run.to_a)
    assert_raise(RuntimeError) { r.add(1, [1]).run }
    assert_raise(RuntimeError) { r.add([1], 1).run }

    arr = (0...10).collect { |x| x }
    assert_equal(arr[(0...3)], r(arr)[(0...3)].run.to_a)
    assert_equal(arr[(0...0)], r(arr)[(0...0)].run.to_a)
    assert_equal(arr[(5...15)], r(arr)[(5...15)].run.to_a)
    assert_equal(arr[(5...-3)], r(arr)[(5...-3)].run.to_a)
    assert_equal(arr[(-5...-3)], r(arr)[(-5...-3)].run.to_a)
    assert_equal(arr[(0..3)], r(arr)[(0..3)].run.to_a)
    assert_equal(arr[(0..0)], r(arr)[(0..0)].run.to_a)
    assert_equal(arr[(5..15)], r(arr)[(5..15)].run.to_a)
    assert_equal(arr[(5..-3)], r(arr)[(5..-3)].run.to_a)
    assert_equal(arr[(-5..-3)], r(arr)[(-5..-3)].run.to_a)

    assert_raise(RuntimeError) { r(1)[(0...0)].run.to_a }
    assert_raise(RuntimeError) { r(arr)[(0.5...0)].run.to_a }
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

  def test_stream
    arr = (0..9).collect { |x| x }
    assert_equal(arr, r(arr).to_stream.to_array.run)
    assert_equal(0, r(arr).to_stream.nth(0).run)
    assert_equal(5, r(arr).to_stream.nth(5).run)
    assert_raise(RuntimeError) { r(arr).to_stream.nth([]).run }
    assert_raise(RuntimeError) { r(arr).to_stream.nth(0.4).run }
    assert_raise(RuntimeError) { r(arr).to_stream.nth(-5).run }
    assert_raise(RuntimeError) { r([0]).to_stream.nth(1).run }
  end

  # from python tests
  def test_stream_fancy
    assert_equal([], limit([], 0))
    assert_equal([], limit([1, 2], 0))
    assert_equal([1], limit([1, 2], 1))
    assert_equal([1, 2], limit([1, 2], 5))
    assert_raise(RuntimeError) { limit([], -1) }

    assert_equal([], skip([], 0))
    assert_equal([], skip([1, 2], 5))
    assert_equal([1, 2], skip([1, 2], 0))
    assert_equal([2], skip([1, 2], 1))

    assert_equal([], distinct([]))
    assert_equal([1, 2, 3], distinct(([1, 2, 3] * 10)))
    assert_equal([1, 2, 3], distinct([1, 2, 3, 2]))
    assert_equal([true, 2, false], distinct([true, 2, false, 2]))
  end

  def test_ordering
    docs = (0..9).map { |n| { "id" => ((100 + n)), "a" => (n), "b" => ((n % 3)) } }
    assert_equal(docs.sort_by { |x| x["a"] }, order(r(docs).to_stream, "a"))
    assert_equal(docs.sort_by { |x| x["a"] }.reverse, order(r(docs).to_stream, ["a", false]))
    assert_equal(docs.select { |x| (x["b"] == 0) }.sort_by { |x| x["a"] },
                 order(r(docs).to_stream.filter("b" => 0), :a))
  end

  # +,-,%,*,/,<,>,<=,>=,eq,ne,any,all
  def test_ops
    assert_equal(8, (r(5) + 3).run)
    assert_equal(8, r(5).add(3).run)
    assert_equal(8, r.add(5, 3).run)
    assert_equal(8, r.add(2, 3, 3).run)

    assert_equal(2, (r(5) - 3).run)
    assert_equal(2, r(5).sub(3).run)
    assert_equal(2, r(5).subtract(3).run)
    assert_equal(2, r.sub(5, 3).run)
    assert_equal(2, r.subtract(5, 3).run)

    assert_equal(2, (r(5) % 3).run)
    assert_equal(2, r(5).mod(3).run)
    assert_equal(2, r(5).modulo(3).run)
    assert_equal(2, r.mod(5, 3).run)
    assert_equal(2, r.modulo(5, 3).run)

    assert_equal(15, (r(5) * 3).run)
    assert_equal(15, r(5).mul(3).run)
    assert_equal(15, r(5).multiply(3).run)
    assert_equal(15, r.mul(5, 3).run)
    assert_equal(15, r.multiply(5, 3).run)
    assert_equal(15, r.multiply(5, 3, 1).run)

    assert_equal(5, (r(15) / 3).run)
    assert_equal(5, r(15).div(3).run)
    assert_equal(5, r(15).divide(3).run)
    assert_equal(5, r.div(15, 3).run)
    assert_equal(5, r.divide(15, 3).run)

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
    assert_equal(false, r.equals(3, 2).run)
    assert_equal(true, r.equals(3, 3).run)

    assert_equal(true, r.ne(3, 2).run)
    assert_equal(false, r.ne(3, 3).run)
    assert_equal(true, r.not(r.equals(3, 2)).run)
    assert_equal(false, r.not(r.equals(3, 3)).run)
    assert_equal(true, r.equals(3, 2).not.run)
    assert_equal(false, r.equals(3, 3).not.run)

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

  # JSON
  def test_json
    assert_equal([1, 2, 3], r.json("[1,2,3]").run)
  end

  # TABLE
  def test_easy_read
    assert_equal(id_sort(tbl.run.to_a), data)
    assert_equal(id_sort(r.db("test").table("tbl").run.to_a), data)
  end

  # IF, JSON, ERROR
  def test_error
    query = r.branch((r.add(1, 2) >= 3), r.json("[1,2,3]"), r.error("unreachable"))
    query_err = r.branch((r.add(1, 2) > 3), r.json("[1,2,3]"), r.error("reachable"))
    assert_equal([1, 2, 3], query.run)
    assert_raise(RuntimeError) { assert_equal(query_err.run) }
  end

  # BOOL, JSON_NULL, ARRAY, ARRAYTOSTREAM
  def test_array
    assert_equal([true, false, nil], r([true, false, nil]).run)
    assert_equal([true, false, nil], r([true, false, nil]).array_to_stream.run.to_a)
    assert_equal([true, false, nil], r([true, false, nil]).to_stream.run.to_a)
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
      tbl.filter { |row| (row[:id] < outer_row[:id]) }.to_array
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

    assert_equal(data.map { true }, tbl.map { |r| r.contains(:id) }.run.to_a)
    assert_equal(data.map { false }, tbl.map { |row| r.not(row.contains("id")) }.run.to_a)
    assert_equal(data.map { false }, tbl.map { |row| r.not(row.contains("id")) }.run.to_a)
    assert_equal(data.map { false }, tbl.map { |row| r.not(row.contains("id")) }.run.to_a)
    assert_equal(data.map { true }, tbl.map { |r| r.contains(:id) }.run.to_a)
    assert_equal(data.map { false }, tbl.map { |r| r.contains("id").not }.run.to_a)
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
    query_234 = query_2345.filter { |row| row[:num].neq(5) }
    query_23 = query_234.filter { |row| r.any(row[:num].eq(2), row[:num].equals(3)) }
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

    assert_equal({ "inserted" => 10, "errors" => 0 }, tbl1.insert((0...10).map { |x| { :id => (x), :a => ((x * 2)) } }).run)
    assert_equal({ "inserted" => 10, "errors" => 0 }, tbl2.insert((0...10).map { |x| { :id => (x), :b => ((x * 100)) } }).run)

    tbl1.eq_join(:a, tbl2).run.each do |pair|
      assert_equal(pair["right"]["id"], pair["left"]["a"])
    end

    assert_equal(tbl1.inner_join(tbl2) { |l, r| l[:a].eq(r[:id]) }.run.to_a.sort_by do |x|
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

  def test_attr_stuff
    assert_raise(RuntimeError) { tbl.get(0, :fake).run }
    assert_raise(RuntimeError) { tbl.get(0, :fake).update { {} }.run }
    assert_raise(RuntimeError) { tbl.get(0, :fake).replace { nil }.run }
    assert_raise(RuntimeError) { tbl.get(0, :fake).delete.run }
  end

  def test_groupby
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_equal({ "skipped" => 0, "errors" => 0, "updated" => 10 }, tbl.update { |row| { :gb => ((row[:id] % 3)) } }.run)
    counts = tbl.group_by(:gb, r.count).run.sort_by { |x| x["group"] }
    sums = tbl.group_by(:gb, r.sum(:id)).run.sort_by { |x| x["group"] }
    avgs = tbl.group_by(:gb, r.avg(:id)).run.sort_by { |x| x["group"] }
    avgs.each_index do |i|
      assert_equal(avgs[i]["reduction"], (sums[i]["reduction"].to_f / counts[i]["reduction"]))
    end
    assert_equal({ "errors" => 0, "deleted" => 0, "inserted" => 0, "modified" => 10 }, tbl.replace { |row| row.unpick(:gb) }.run)
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  def test_too_big_key
    assert_not_nil(tbl.insert({ :id => (("a" * 1000)) }, :upsert).run["first_error"])
  end

  def test_random_insert_regressions
    assert_not_nil(tbl.insert(true).run["first_error"])
    assert_not_nil(tbl.insert([true, true]).run["first_error"])
    assert_not_nil(tbl.insert(r([true]).to_stream).run["first_error"])
  end

  def test_key_generation
    assert_equal(data, tbl.order_by(:id).run.to_a)
    res = tbl.insert([[], {}, { :a => 1 }, { :id => -1, :a => 2 }]).run
    assert_equal(1, res["errors"])
    assert_equal(3, res["inserted"])
    assert_equal(2, res["generated_keys"].length)
    assert_equal(1, tbl.get(res["generated_keys"][0]).delete.run["deleted"])
    assert_equal(2, tbl.filter { |row| (row[:id] < 0).|((row[:id] > 1000)) }.delete.run["deleted"])
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  # SLICE
  def test_slice_streams
    arr = [0, 1, 2, 3, 4, 5]
    assert_equal(1, r(arr)[1].run)
    assert_equal(1, r(arr).to_stream[1].run)

    assert_equal(r(arr)[(2..-1)].run.to_a, r(arr)[(2...6)].run.to_a)
    assert_equal(r(arr).to_stream[(2..-1)].run.to_a, r(arr).to_stream[(2...6)].run.to_a)

    assert_equal(r(arr)[(2..4)].run.to_a, r(arr)[(2...5)].run.to_a)
    assert_equal(r(arr).to_stream[(2..4)].run.to_a, r(arr).to_stream[(2...5)].run.to_a)

    assert_raise(RuntimeError) { r(arr)[(2...0)].run.to_a }
    assert_raise(RuntimeError) { r(arr).to_stream[(2...-1)].run.to_a }
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
    assert_equal(data, tbl.order_by(r.asc(:id)).run.to_a)
    assert_equal(data.reverse, tbl.order_by([:id, false]).run.to_a)
    assert_equal(data.reverse, tbl.order_by(r.desc(:id)).run.to_a)
    query = tbl.map { |x| r(:id => (x[:id]), :num => (x[:id].mod(2))) }.order_by(:num, [:id, false])
    want = data.map { |o| o["id"] }.sort_by { |n| (((n % 2) * data.length) - n) }
    assert_equal(want, query.run.to_a.map { |o| o["id"] })
    assert_equal(tbl.order_by(r.asc(:id)).run.to_a, tbl.order_by(:id).run.to_a)
  end

  # CONCATMAP, DISTINCT
  def test_concatmap
    assert_equal([1, 2, 1, 2, 1, 2], r([1, 2, 3]).concat_map { r([1, 2]).to_stream }.to_stream.run.to_a)
    assert_equal([1, 2], r([[1], [2]]).concat_map { |x| x }.run.to_a)
    assert_raise(RuntimeError) { r([[1], 2]).concat_map { |x| x }.run.to_a }
    assert_raise(RuntimeError) { r(1).concat_map { |x| x }.run.to_a }
    assert_equal([1, 2], r([[1], [2]]).concat_map { |x| x }.run.to_a)
    query = tbl.concat_map { |row| tbl.map { |row2| (row2[:id] * row[:id]) } }.distinct
    nums = data.map { |o| o["id"] }
    want = nums.map { |n| nums.map { |m| (n * m) } }.flatten(1).uniq
    assert_equal(want.sort, query.run.to_a.sort)
  end

  # RANGE
  def test_range
    assert_raise(RuntimeError) { tbl.between(1, r([3])).run.to_a }
    assert_raise(RuntimeError) { tbl.between(2, 1).run.to_a }
    assert_equal(data[(1..3)], id_sort(tbl.between(1, 3).run.to_a))
    assert_equal(data[(3..3)], id_sort(tbl.between(3, 3).run.to_a))
    assert_equal(data[(2..-1)], id_sort(tbl.between(2, nil).run.to_a))
    assert_equal(data[(1..3)], id_sort(tbl.between(1, 3).run.to_a))
    assert_equal(data[(0..4)], id_sort(tbl.between(nil, 4).run.to_a))

    assert_equal(data[(1..3)], id_sort(tbl.to_array.between(1, 3).run.to_a))
    assert_equal(data[(2..-1)], id_sort(tbl.to_array.between(2, nil).run.to_a))
    assert_equal(data[(1..3)], id_sort(tbl.to_array.between(1, 3).run.to_a))
    assert_equal(data[(0..4)], id_sort(tbl.to_array.between(nil, 4).run.to_a))

    assert_equal(data[(1..3)], id_sort(r(data).between(1, 3).run.to_a))
    assert_equal(data[(2..-1)], id_sort(r(data).between(2, nil).run.to_a))
    assert_equal(data[(1..3)], id_sort(r(data).between(1, 3).run.to_a))
    assert_equal(data[(0..4)], id_sort(r(data).between(nil, 4).run.to_a))

    assert_raise(RuntimeError) { r([1]).between(1, 3).run.to_a }
    assert_raise(RuntimeError) { r([1, 2]).between(1, 3).run.to_a }
  end

  # GROUPEDMAPREDUCE
  #TODO: Add tests once issue #922 is resolved.
  def test_groupedmapreduce
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_equal(data, tbl.to_array.order_by(:id).run.to_a)
    assert_equal([0, 1], r([{ :id => 1 }, { :id => 0 }]).order_by(:id).run.to_a.map { |x| x["id"] })
    assert_raise(RuntimeError) { r(1).order_by(:id).run.to_a }
    assert_raise(RuntimeError) { r([1]).nth(0).order_by(:id).run.to_a }
    assert_raise(RuntimeError) { r([1]).order_by(:id).run.to_a }
    assert_raise(RuntimeError) { r([{ :num => 1 }]).order_by(:id).run.to_a }
    assert_equal([], r([]).order_by(:id).run.to_a)

    gmr = tbl.grouped_map_reduce(lambda { |row| (row[:id] % 4) }, lambda { |row| row[:id] }, 0, lambda { |a, b| (a + b) })
    gmr2 = tbl.grouped_map_reduce(lambda { |r| (r[:id] % 4) }, lambda { |r| r[:id] }, 0, lambda { |a, b| (a + b) })
    gmr3 = tbl.to_array.grouped_map_reduce(lambda { |row| (row[:id] % 4) }, lambda { |row| row[:id] }, 0, lambda { |a, b| (a + b) })
    gmr4 = r(data).grouped_map_reduce(lambda { |row| (row[:id] % 4) }, lambda { |row| row[:id] }, 0, lambda { |a, b| (a + b) })
    assert_equal(gmr2.run.to_a, gmr.run.to_a)
    assert_equal(gmr3.run.to_a, gmr.run.to_a)
    assert_equal(gmr4.run.to_a, gmr.run.to_a)
    gmr5 = r([data]).grouped_map_reduce(lambda { |row| (row[:id] % 4) }, lambda { |row| row[:id] }, 0, lambda { |a, b| (a + b) })
    gmr6 = r(1).grouped_map_reduce(lambda { |row| (row[:id] % 4) }, lambda { |row| row[:id] }, 0, lambda { |a, b| (a + b) })
    assert_raise(RuntimeError) { gmr5.run.to_a }
    assert_raise(RuntimeError) { gmr6.run.to_a }
    gmr.run.to_a.each do |obj|
      want = data.map { |x| x["id"] }.select { |x| ((x % 4) == obj["group"]) }.reduce(0, :+)
      assert_equal(want, obj["reduction"])
    end
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  # NTH
  def test_nth
    assert_equal(data[2], tbl.order_by(:id).nth(2).run)
  end

  # JAVASCRIPT
  def test_javascript
    assert_equal(1, r.js("1").run)
    assert_equal(3, r.js("1").add(r.js("2")).run)
    assert_equal(4, r.js("2+2").run)
    assert_equal("cows", r.js("\"cows\"").run)
    assert_equal([1, 2, 3], r.js("[1,2,3]").run)
    assert_equal({}, r.js("({})").run)
    assert_equal({ "a" => "whee" }, r.js("({a: \"whee\"})").run)
    assert_equal({}, r.js("this").run)

    assert_equal(0, r.js("0;").run)
    assert_raise(RuntimeError) { r.js("undefined").run }
    assert_raise(RuntimeError) { r.js(";").run }
    assert_raise(RuntimeError) { r.js("var x = {}; x.x = x; return x;").run }
  end

  # JAVASCRIPT
  def test_javascript_vars
    assert_equal(2, r.let([["x", 2]]) { r.js("x") }.run)
    assert_equal(5, r.let([["x", 2], ["y", 3]]) { r.js("x+y") }.run)
    assert_equal(id_sort(tbl.run.to_a), id_sort(tbl.map { |x| r.js("#{x}") }.run.to_a))
    assert_equal(id_sort(tbl.run.to_a), id_sort(tbl.map { r.js("this") }.run.to_a))
    assert_equal(tbl.map { |r| r[:num] }.run.to_a.sort, tbl.map { |x| r.js("#{x}.num") }.run.to_a.sort)
    assert_equal(tbl.map { |r| r[:num] }.run.to_a.sort, tbl.map { r.js("this.num") }.run.to_a.sort)
    assert_equal(id_sort(tbl.filter { |r| (r[:id] < 5) }.run.to_a), id_sort(tbl.filter { |x| r.js("#{x}.id < 5") }.run.to_a))
    assert_equal(id_sort(tbl.filter { |r| (r[:id] < 5) }.run.to_a), id_sort(tbl.filter { r.js("this.id < 5") }.run.to_a))
  end

  def test_pluck
    e = r([{ "a" => 1, "b" => 2, "c" => 3 }, { "a" => 4, "b" => 5, "c" => 6 }])
    assert_equal([{}, {}], e.pluck.run)
    assert_equal([{ "a" => 1 }, { "a" => 4 }], e.pluck("a").run)
    assert_equal([{ "a" => 1, "b" => 2 }, { "a" => 4, "b" => 5 }], e.pluck("a", "b").run)
  end

  # PICKATTRS, # UNION, # LENGTH
  def test_pickattrs
    q1 = r.union(tbl.map { |r| r.pick(:id, :name) }, tbl.map { |row| row.pick(:id, :num) })
    q2 = r.union(tbl.map { |r| r.pick(:id, :name) }, tbl.map { |row| row.pick(:id, :num) })
    q1v = q1.run.to_a.sort_by { |x| ((x["name"].to_s + ",") + x["id"].to_s) }
    assert_equal(q2.run.to_a.sort_by { |x| ((x["name"].to_s + ",") + x["id"].to_s) }, q1v)

    len = data.length

    assert_equal((2 * len), q2.count.run)
    assert_equal(Array.new(len, nil), q1v.map { |o| o["num"] }[(len..((2 * len) - 1))])
    assert_equal(Array.new(len, nil), q1v.map { |o| o["name"] }[(0..(len - 1))])
  end

  def test_pointdelete
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_equal({ "deleted" => 1 }, tbl.get(0).delete.run)
    assert_equal({ "deleted" => 0 }, tbl.get(0).delete.run)
    assert_equal(data[(1..-1)], tbl.order_by(:id).run.to_a)
    assert_equal({ "inserted" => 1, "errors" => 0 }, tbl.insert(data[0], :upsert).run)
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  def test_write_python # from python tests
    tbl.delete.run
    docs = [{ "a" => 3, "b" => 10, "id" => 1 },
            { "a" => 9, "b" => -5, "id" => 2 },
             { "a" => 9, "b" => 3, "id" => 3 }]
    assert_equal({ "inserted" => (docs.length), "errors" => 0 }, tbl.insert(docs, :upsert).run)
    docs.each { |doc| assert_equal(doc, tbl.get(doc["id"]).run) }
    assert_equal([3, 9].sort, tbl.distinct(:a).run.to_a.sort)
    assert_equal([docs[0]], tbl.filter("a" => 3).run.to_a)
    assert_raise(RuntimeError) { tbl.filter("a" => ((tbl.count + ""))).run.to_a }
    assert_not_nil(tbl.insert({ "a" => 3 }, :upsert).run["first_error"])
    assert_equal(nil, tbl.get(0).run)
    assert_equal({ "inserted" => 1, "errors" => 0 }, tbl.insert({ :id => 100, :text => "\u{30b0}\u{30eb}\u{30e1}" }, :upsert).run)
    assert_equal("\u{30b0}\u{30eb}\u{30e1}", tbl.get(100)[:text].run)
    tbl.delete.run
    docs = [{ "id" => 1 }, { "id" => 2 }]
    assert_equal({ "inserted" => (docs.length), "errors" => 0 }, tbl.insert(docs, :upsert).run)
    assert_equal({ "deleted" => 1 }, tbl.limit(1).delete.run)
    assert_equal(1, tbl.run.to_a.length)
    tbl.delete.run
    docs = (0...4).map { |n| { "id" => ((100 + n)), "a" => (n), "b" => ((n % 3)) } }
    assert_equal({ "inserted" => (docs.length), "errors" => 0 }, tbl.insert(docs, :upsert).run)
    assert_equal({ "inserted" => (docs.length), "errors" => 0 }, tbl.insert(r(docs).to_stream, :upsert).run)
    docs.each { |doc| assert_equal(doc, tbl.get(doc["id"]).run) }
    tbl.delete.run
    docs = (0...10).map { |n| { "id" => ((100 + n)), "a" => (n), "b" => ((n % 3)) } }
    assert_equal({ "inserted" => (docs.length), "errors" => 0 }, tbl.insert(docs, :upsert).run)

    # TODO: still an error?
    # assert_raise(ArgumentError){r[:a] == 5}
    # assert_raise(ArgumentError){r[:a] != 5}
    assert_equal(docs.select { |x| (x["a"] < 5) }, id_sort(tbl.filter { |r| (r[:a] < 5) }.run.to_a))
    assert_equal(docs.select { |x| (x["a"] <= 5) }, id_sort(tbl.filter { |r| (r[:a] <= 5) }.run.to_a))
    assert_equal(docs.select { |x| (x["a"] > 5) }, id_sort(tbl.filter { |r| (r[:a] > 5) }.run.to_a))
    assert_equal(docs.select { |x| (x["a"] >= 5) }, id_sort(tbl.filter { |r| (r[:a] >= 5) }.run.to_a))
    assert_equal(docs.select { |x| (x["a"] == x["b"]) }, id_sort(tbl.filter { |r| r[:a].eq(r[:b]) }.run.to_a))

    assert_equal(r(-5).run, -r(5).run)
    assert_equal(r(5).run, +r(5).run)

    assert_equal(7, (r(3) + 4).run)
    assert_equal(-1, (r(3) - 4).run)
    assert_equal(12, (r(3) * 4).run)
    assert_equal((3.0 / 4), (r(3) / 4).run)
    assert_equal((3 % 2), (r(3) % 2).run)
    assert_raise(TypeError) { (4 + r(3)) }
    assert_raise(TypeError) { (4 - r(3)) }
    assert_raise(TypeError) { (4 * r(3)) }
    assert_raise(TypeError) { (4 / r(3)) }
    assert_raise(TypeError) { (4 % r(3)) }

    assert_equal(84, (((r(3) + 4) * -r(6)) * (r(-5) + 3)).run)

    tbl.delete.run
    docs = (0...10).map { |n| { "id" => ((100 + n)), "a" => (n), "b" => ((n % 3)) } }
    assert_equal({ "inserted" => (docs.length), "errors" => 0 }, tbl.insert(docs, :upsert).run)
    assert_equal({ "modified" => (docs.length), "inserted" => 0, "deleted" => 0, "errors" => 0, "errors" => 0 }, tbl.replace { |x| x }.run)
    assert_equal(docs, id_sort(tbl.run.to_a))
    assert_equal({ "updated" => 0, "skipped" => 10, "errors" => 0 }, tbl.update { nil }.run)

    res = tbl.update { 1 }.run
    assert_equal(10, res["errors"])
    res = tbl.update { |row| row[:id] }.run
    assert_equal(10, res["errors"])
    res = tbl.update { |r| r[:id] }.run
    assert_equal(10, res["errors"])

    res = tbl.update { |row| r.branch((row[:id] % 2).eq(0), { :id => -1 }, row) }.run
    assert_not_nil(res["first_error"])
    filtered_res = res.reject { |k, v| (k == "first_error") }
    assert_not_equal(filtered_res, res)
    assert_equal({ "updated" => 5, "skipped" => 0, "errors" => 5 }, filtered_res)

    assert_equal(docs, id_sort(tbl.run.to_a))
  end

  def test_getitem
    arr = (0...10).map { |x| x }
    [(0..-1), (2..-1), (0...2), (-1..-1), (0...-1), (3...5), 3, -1].each do |rng|
      assert_equal(arr[rng], r(arr)[rng].run)
    end
    obj = { :a => 3, :b => 4 }
    [:a, :b].each { |attr| assert_equal(obj[attr], r(obj)[attr].run) }
    assert_raise(RuntimeError) { r(obj)[:c].run }
  end

  def test_contains
    obj = r(:a => 1, :b => 2)
    assert_equal(true, obj.contains(:a).run)
    assert_equal(false, obj.contains(:c).run)
    assert_equal(true, obj.contains(:a, :b).run)
    assert_equal(false, obj.contains(:a, :c).run)
  end

  def test_stream_getitem
    arr = (0...10).map { |x| x }
    [(0..-1), (3..-1), (0...3), (4...6)].each do |rng|
      assert_equal(arr[rng], r(arr).to_stream[rng].run.to_a)
    end
    [3].each { |rng| assert_equal(arr[rng], r(arr).to_stream[rng].run) }
    assert_raise(ArgumentError) { r(arr).to_stream[(4..."a")].run.to_a }
  end

  #FOREACH
  def test_array_foreach
    assert_equal(data, id_sort(tbl.run.to_a))
    assert_equal({ "skipped" => 0, "updated" => 6, "errors" => 0 }, r([2, 3, 4]).for_each do |x|
      [tbl.get(x).update { { :num => 0 } }, tbl.get((x * 2)).update { { :num => 0 } }]
    end.run)
    assert_equal({ "skipped" => 4, "updated" => 6, "errors" => 0 }, tbl.to_array.for_each do |row|
      tbl.filter { |r| r[:id].eq(row[:id]) }.update do |row|
        r.branch(row[:num].eq(0), { :num => (row[:id]) }, nil)
      end
    end.run)
    assert_equal(data, id_sort(tbl.run.to_a))
  end

  #FOREACH
  def test_foreach_multi
    db_name = ("foreach_multi" + rand((2 ** 64)).to_s)
    table_name = ("foreach_multi_tbl" + rand((2 ** 64)).to_s)
    table_name_2 = (table_name + rand((2 ** 64)).to_s)
    table_name_3 = (table_name_2 + rand((2 ** 64)).to_s)
    table_name_4 = (table_name_3 + rand((2 ** 64)).to_s)
    assert_equal(nil, r.db_create(db_name).run)
    assert_equal(nil, r.db(db_name).table_create(table_name).run)
    assert_equal(nil, r.db(db_name).table_create(table_name_2).run)
    assert_equal(nil, r.db(db_name).table_create(table_name_3).run)
    assert_equal(nil, r.db(db_name).table_create(table_name_4).run)
    tbl = r.db(db_name).table(table_name)
    tbl2 = r.db(db_name).table(table_name_2)
    tbl3 = r.db(db_name).table(table_name_3)
    tbl4 = r.db(db_name).table(table_name_4)
    assert_equal({ "inserted" => 10, "errors" => 0 }, tbl.insert(data, :upsert).run)
    assert_equal({ "inserted" => 10, "errors" => 0 }, tbl2.insert(data, :upsert).run)
    query = tbl.for_each do |row|
      tbl2.for_each do |row2|
        tbl3.insert({ "id" => (((row[:id] * 1000) + row2[:id])) }, :upsert)
      end
    end
    assert_equal({ "inserted" => 100, "errors" => 0 }, query.run)
    query = tbl.for_each do |row|
      tbl2.filter { |r| r[:id].eq((row[:id] * row[:id])) }.delete
    end
    assert_equal({ "deleted" => 4 }, query.run)
    query = tbl.for_each do |row|
      tbl2.for_each do |row2|
        tbl3.filter { |r| r[:id].eq(((row[:id] * 1000) + row2[:id])) }.delete
      end
    end
    assert_equal({ "deleted" => 60 }, query.run)
    assert_equal({ "inserted" => 10, "errors" => 0 }, tbl.for_each { |row| tbl4.insert(row, :upsert) }.run)
    assert_equal({ "deleted" => 6 }, tbl2.for_each { |row| tbl4.filter { |r| r[:id].eq(row[:id]) }.delete }.run)

    # TODO: Pending resolution of issue #930
    # assert_equal(tbl3.order_by(:id).run,
    #              r.let([['x', tbl2.concat_map{|row|
    #                        tbl.filter{
    #                          r[:id].neq(row[:id])
    #                        }
    #                      }.map{r[:id]}.distinct.to_array]],
    #                    tbl.concat_map{|row|
    #                      r[:$x].to_stream.map{|id| row[:id]*1000+id}}).run)
  ensure
    r.db_drop(db_name).run
  end

  #FOREACH
  def test_fancy_foreach
    db_name = rand((2 ** 64)).to_s
    table_name = rand((2 ** 64)).to_s

    assert_equal(nil, r.db_create(db_name).run)
    assert_equal(nil, r.db(db_name).table_create(table_name).run)
    tbl = r.db(db_name).table(table_name)
    assert_equal({ "inserted" => 10, "errors" => 0 }, tbl.insert(data, :upsert).run)
    tbl.insert({ :id => 11, :broken => (true) }, :upsert).run
    tbl.insert({ :id => 12, :broken => (true) }, :upsert).run
    tbl.insert({ :id => 13, :broken => (true) }, :upsert).run

    query = tbl.order_by(:id).for_each do |row|
      [tbl.update { |row2| r.branch(row[:id].eq(row2[:id]), { :num => ((row2[:num] * 2)) }, nil)},
        tbl.filter { |r| r[:id].eq(row[:id]) }.replace { |row| r.branch((row[:id] % 2).eq(1), nil, row) },
        tbl.get(0).delete, tbl.get(12).delete]
    end

    res = query.run

    assert_equal(2, res["errors"])
    assert_not_nil(res["first_error"])
    assert_equal(9, res["deleted"])
    assert_equal(10, res["updated"])
    assert_equal(5, res["modified"])
    assert_equal([{ "num" => 4, "id" => 2, "name" => "2" }, { "num" => 8, "id" => 4, "name" => "4" }, { "num" => 12, "id" => 6, "name" => "6" }, { "num" => 16, "id" => 8, "name" => "8" }], tbl.order_by(:id).run.to_a)

  ensure
    r.db_drop(db_name).run
  end

  def test_bad_primary_key_type
    assert_not_nil(tbl.insert({ :id => ([]) }, :upsert).run["first_error"])
    assert_raise(RuntimeError) { tbl.get(100).replace { { :id => ([]) } }.run }
  end

  def test_big_between
    data_100 = (0..99).map { |x| { :id => (x) } }
    assert_equal({ "inserted" => 100, "errors" => 0 }, tbl.insert(data_100, :upsert).run)
    assert_equal([{ "id" => 1 }, { "id" => 2 }], id_sort(tbl.between(1, 2).run.to_a))
    assert_equal({ "deleted" => 100 }, tbl.delete.run)
    assert_equal({ "inserted" => 10, "errors" => 0 }, tbl.insert(data, :upsert).run)
    assert_equal(data, id_sort(tbl.run.to_a))
  end

  #POINTMUTATE
  def test_mutate_edge_cases
    res = tbl.replace { 1 }.run
    assert_equal(10, res["errors"])
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_equal({ "deleted" => 1, "inserted" => 0, "modified" => 0, "errors" => 0, "errors" => 0 },
                 tbl.get(0).replace { nil }.run)
    assert_equal(data[(1..-1)], tbl.order_by(:id).run.to_a)
    assert_equal({ "deleted" => 0, "inserted" => 0, "modified" => 0, "errors" => 0, "errors" => 0 },
                 tbl.get(0).replace { nil }.run)
    assert_equal(data[(1..-1)], tbl.order_by(:id).run.to_a)
    #assert_equal(tbl.get(0).replace{|row| r.branch(row.eq(nil), data[0], data[1])}.run,
    assert_equal({ "deleted" => 0, "inserted" => 1, "modified" => 0, "errors" => 0, "errors" => 0 },
                 tbl.get(0).replace { data[0] }.run)
    assert_raise(RuntimeError) { tbl.get(-1).replace { { :id => ([]) } }.run }
    assert_raise(RuntimeError) { tbl.get(-1).replace { { :id => 0 } }.run }
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_raise(RuntimeError) do
      tbl.get(0).replace { |row| r.branch(row.eq(nil), data[0], data[1]) }.run.to_a
    end
    assert_equal({ "deleted" => 0, "inserted" => 0, "modified" => 1, "errors" => 0, "errors" => 0 },
                 tbl.get(0).replace { |row| r.branch(row.eq(nil), data[1], data[0]) }.run)
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  def test_insert
    assert_equal(data, tbl.order_by(:id).run.to_a)
    data2 = (data + data.map { |x| { "id" => ((x["id"] + data.length)) } })

    res = tbl.insert(data2).run
    assert_equal(data.length, res["inserted"])
    assert_equal(data.length, res["errors"])
    assert_not_nil(res["first_error"])

    assert_equal(data2, tbl.order_by(:id).run.to_a)
    assert_equal({ "deleted" => (data2.length) }, tbl.delete.run)

    res = tbl.insert(data).run
    assert_equal(data.length, res["inserted"])
    assert_equal(0, res["errors"])
    assert_nil(res["first_error"])

    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  #POINTUPDATE
  def test_update_edge_cases
    assert_equal(data, tbl.order_by(:id).run.to_a)
    assert_equal({ "skipped" => 1, "updated" => 0, "errors" => 0 },
                 tbl.get(0).update { nil }.run)
    assert_equal({ "skipped" => 1, "updated" => 0, "errors" => 0 },
                 tbl.get(11).update { nil }.run)
    assert_equal({ "skipped" => 1, "updated" => 0, "errors" => 0 },
                 tbl.get(11).update { {} }.run)
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  #FOREACH
  def test_foreach_error
    assert_equal(data, tbl.order_by(:id).run.to_a)
    query = tbl.for_each { |row| tbl.get(row[:id]).update { { :id => 0 } } }
    res = query.run
    assert_equal(9, res["errors"])
    assert_not_nil(res["first_error"])
    assert_equal(data, tbl.order_by(:id).run.to_a)
  end

  def test_write
    db_name = rand((2 ** 64)).to_s
    table_name = rand((2 ** 64)).to_s
    orig_dbs = r.db_list.run

    assert_equal(nil, r.db_create(db_name).run)
    assert_raise(RuntimeError) { r.db_create(db_name).run }

    new_dbs = r.db_list.run

    assert_equal((orig_dbs.length + 1), new_dbs.length)
    assert_equal(nil, r.db(db_name).table_create(table_name).run)
    assert_raise(RuntimeError) { r.db(db_name).table_create(table_name).run }
    assert_equal([table_name], r.db(db_name).table_list.run)
    assert_equal({ "inserted" => 1, "errors" => 0 },
                 r.db(db_name).table(table_name).insert({ :id => 0 }, :upsert).run)
    assert_equal([{ "id" => 0 }], r.db(db_name).table(table_name).run.to_a)
    assert_equal(nil, r.db(db_name).table_create((table_name + table_name)).run)
    assert_equal(2, r.db(db_name).table_list.run.length)
    assert_raise(ArgumentError) { r.db("").table("").run.to_a }
    assert_raise(RuntimeError) { r.db("test").table(table_name).run.to_a }
    assert_raise(ArgumentError) { r.db(db_name).table("").run.to_a }
    assert_equal(nil, r.db(db_name).table_drop((table_name + table_name)).run)
    assert_raise(RuntimeError) do
      r.db(db_name).table((table_name + table_name)).run.to_a
    end

    assert_equal(nil, r.db_drop(db_name).run)
    assert_equal(orig_dbs.sort, r.db_list.run.sort)
    assert_raise(RuntimeError) { r.db(db_name).table(table_name).run.to_a }
    assert_equal(nil, r.db_create(db_name).run)
    assert_raise(RuntimeError) { r.db(db_name).table(table_name).run.to_a }
    assert_equal(nil, r.db(db_name).table_create(table_name).run)
    assert_equal([], r.db(db_name).table(table_name).run.to_a)

    tbl2 = r.db(db_name).table(table_name)
    len = data.length

    # insert, UPDATE, :upsert
    assert_equal(len, tbl2.insert(data, :upsert).run["inserted"])
    assert_equal((len * 2), tbl2.insert((data + data), :upsert).run["inserted"])
    assert_equal(data, id_sort(tbl2.run.to_a))
    assert_equal(1, tbl2.insert({ :id => 0, :broken => (true), "errors" => 0 }, :upsert).run["inserted"])
    assert_equal(1, tbl2.insert({ :id => 1, :broken => (true), "errors" => 0 }, :upsert).run["inserted"])
    assert_equal(data[(2..(len - 1))], id_sort(tbl2.run.to_a)[(2..(len - 1))])
    assert_equal(true, id_sort(tbl2.run.to_a)[0]["broken"])
    assert_equal(true, id_sort(tbl2.run.to_a)[1]["broken"])
    mutate = tbl2.filter { |r| r[:id].eq(0) }.replace { data[0] }.run
    assert_equal({ "modified" => 1, "inserted" => 0, "deleted" => 0, "errors" => 0, "errors" => 0 }, mutate)
    update = tbl2.update { |x| r.branch(x.contains(:broken), data[1], x) }.run
    assert_equal({ "errors" => 0, "updated" => (len), "skipped" => 0 }, update)
    mutate = tbl2.replace do |x|
      { :id => (x[:id]), :num => (x[:num]), :name => (x[:name]) }
    end.run
    assert_equal({ "modified" => 10, "inserted" => 0, "deleted" => 0, "errors" => 0, "errors" => 0 }, mutate)
    assert_equal(data, id_sort(tbl2.run.to_a))
    mutate = tbl2.replace do |row|
      r.branch(row[:id].eq(4).|(row[:id].eq(5)), { :id => -1 }, row)
    end.run
    assert_not_nil(mutate["first_error"])
    filtered_mutate = mutate.reject { |k, v| (k == "first_error") }
    assert_equal({ "modified" => 8, "inserted" => 0, "deleted" => 0, "errors" => 2 }, filtered_mutate)
    assert_equal({ "updated" => 1, "skipped" => 0, "errors" => 0 }, tbl2.get(5).update { { :num => "5" } }.run)
    mutate = tbl2.replace { |row| r.merge(row, :num => ((row[:num] * 2))) }.run
    assert_not_nil(mutate["first_error"])
    filtered_mutate = mutate.reject { |k, v| (k == "first_error") }
    assert_equal({ "modified" => 9, "inserted" => 0, "deleted" => 0, "errors" => 1 }, filtered_mutate)
    assert_equal("5", tbl2.get(5).run["num"])
    assert_equal({ "modified" => 10, "inserted" => 0, "deleted" => 0, "errors" => 0 }, tbl2.replace do |row|
      r.branch(row[:id].eq(5), data[5], row.merge(:num => ((row[:num] / 2))))
    end.run)
    assert_equal(data, id_sort(tbl2.run.to_a))
    update = tbl2.get(0).update { { :num => 2 } }.run
    assert_equal({ "errors" => 0, "updated" => 1, "skipped" => 0 }, update)
    assert_equal(2, tbl2.get(0).run["num"])
    update = tbl2.get(0).update { nil }.run
    assert_equal({ "errors" => 0, "updated" => 0, "skipped" => 1 }, update)
    assert_equal({ "errors" => 0, "inserted" => 0, "deleted" => 0, "modified" => 1, "errors" => 0 }, tbl2.get(0).replace { data[0] }.run)
    assert_equal(data, id_sort(tbl2.run.to_a))

    # DELETE
    assert_equal(5, tbl2.filter { |r| (r[:id] < 5) }.delete.run["deleted"])
    assert_equal(data[(5..-1)], id_sort(tbl2.run.to_a))
    assert_equal((len - 5), tbl2.delete.run["deleted"])
    assert_equal([], tbl2.run.to_a)

    # insert
    assert_equal(len, tbl2.insert(r(data).to_stream, :upsert).run["inserted"])
    tbl2.insert(data, :upsert).run

    assert_equal(data, id_sort(tbl2.run.to_a))

    # MUTATE
    assert_equal({ "modified" => (len), "inserted" => 0, "deleted" => 0, "errors" => 0, "errors" => 0 }, tbl2.replace { |row| row }.run)
    assert_equal(data, id_sort(tbl2.run.to_a))
    assert_equal({ "modified" => 5, "inserted" => 0, "deleted" => ((len - 5)), "errors" => 0, "errors" => 0 }, tbl2.replace { |row| r.branch((row[:id] < 5), nil, row) }.run)
    assert_equal(data[(5..-1)], id_sort(tbl2.run.to_a))
    assert_equal({ "inserted" => 5, "errors" => 0 }, tbl2.insert(data[(0...5)], :upsert).run)

    # FOREACH, POINTDELETE
    # TODO_SERVER: Return value of foreach should change (Issue #874)
    query = tbl2.for_each do |row|
      [tbl2.get(row[:id]).delete, tbl2.insert(row, :upsert)]
    end
    assert_equal({ "deleted" => 10, "inserted" => 10, "errors" => 0 }, query.run)
    assert_equal(data, id_sort(tbl2.run.to_a))
    tbl2.for_each { |row| tbl2.get(row[:id]).delete }.run
    assert_equal([], id_sort(tbl2.run.to_a))

    tbl2.insert(data, :upsert).run
    assert_equal(data, id_sort(tbl2.run))
    query = tbl2.get(0).update { { :id => 0, :broken => 5 } }
    assert_equal({ "updated" => 1, "errors" => 0, "skipped" => 0 }, query.run)
    query = tbl2.insert(data[0], :upsert)
    assert_equal({ "inserted" => 1, "errors" => 0 }, query.run)
    assert_equal(data, id_sort(tbl2.run.to_a))

    assert_equal({ "modified" => 0, "inserted" => 0, "deleted" => 1, "errors" => 0, "errors" => 0 },
                 tbl2.get(0).replace { nil }.run)
    assert_equal(data[(1..-1)], id_sort(tbl2.run.to_a))
    assert_raise(RuntimeError) { tbl2.get(1).replace { { :id => -1 } }.run.to_a }
    assert_raise(RuntimeError) do
      tbl2.get(1).replace { |row| { :name => ((row[:name] * 2)) } }.run.to_a
    end
    assert_equal({ "modified" => 1, "inserted" => 0, "deleted" => 0, "errors" => 0, "errors" => 0 },
                 tbl2.get(1).replace { |row| row.merge(:num => 2) }.run)
    assert_equal(2, tbl2.get(1).run["num"])
    assert_equal({ "inserted" => 2, "errors" => 0 },
                 tbl2.insert(data[(0...2)], :upsert).run)

    assert_equal(data, id_sort(tbl2.run.to_a))
  ensure
    r.db_drop(db_name).run
  end

  def test_close_and_reconnect
    assert_nothing_raised { c.close }
    c.reconnect
  end
end
