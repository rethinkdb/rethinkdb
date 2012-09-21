# -*- coding: utf-8 -*-
# load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/rethinkdb.rb'
# filter might work, merge into master and check
# random, sample
# reduce

# BIG TODO:
#   * Make Connection work with clusters, minimize network hops,
#     etc. etc. like python client does.

# TODO:
# Union vs. +
# Figure out how to use:
#   * GROUPEDMAPREDUCE
# Add tests once completed/fixed:
#   * REDUCE
#   * FOREACH

################################################################################
#                                 CONNECTION                                   #
################################################################################

$LOAD_PATH.unshift('./rethinkdb')
require 'rethinkdb.rb'
require 'test/unit'
$port_base = ARGV[0].to_i # 0 if none given
class ClientTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  def rdb
    @@c.use('test')
    r.table('Welcome-rdb')
  end
  @@c = RethinkDB::Connection.new('localhost', $port_base + 12346)
  def c; @@c; end
  def id_sort x; x.sort_by{|y| y['id']}; end

  def test_numops
    assert_raise(RuntimeError){r.div(1,0).run}
    assert_raise(RuntimeError){r.div(0,0).run}
    assert_raise(RuntimeError){r.mul(1e200,1e300).run}
    assert_equal(r.mul(1e100,1e100).run, 1e200)
  end

  def test_cmp # from python tests
    assert_equal(r.eq(3, 3).run, true)
    assert_equal(r.eq(3, 4).run, false)

    assert_equal(r.ne(3, 3).run, false)
    assert_equal(r.ne(3, 4).run, true)

    assert_equal(r.gt(3, 2).run, true)
    assert_equal(r.gt(3, 3).run, false)

    assert_equal(r.ge(3, 3).run, true)
    assert_equal(r.ge(3, 4).run, false)

    assert_equal(r.lt(3, 3).run, false)
    assert_equal(r.lt(3, 4).run, true)

    assert_equal(r.le(3, 3).run, true)
    assert_equal(r.le(3, 4).run, true)
    assert_equal(r.le(3, 2).run, false)

    assert_equal(r.eq(1, 1, 2).run, false)
    assert_equal(r.eq(5, 5, 5).run, true)

    assert_equal(r.lt(3, 4).run, true)
    assert_equal(r.lt(3, 4, 5).run, true)
    assert_equal(r.lt(5, 6, 7, 7).run, false)

    assert_equal(r.eq("asdf", "asdf").run, true)
    assert_equal(r.eq("asd", "asdf").run, false)
    assert_equal(r.lt("a", "b").run, true)

    assert_equal(r.eq(true, true).run, true)
    assert_equal(r.lt(false, true).run, true)

    assert_equal(r.lt([], false, true, nil, 1, {}, "").run, true)
  end

  def test_without
    assert_equal(r.expr({:a => 1, :b => 2, :c => 3}).without(:a, :c).run, {'b' => 2})
    assert_equal(rdb.orderby(:id).map{r[:$_].without(:name, :num)}.run.to_a,
                 rdb.orderby(:id).map{|row| row.without(:name).without(:num)}.run.to_a)
    assert_equal(r.expr({:a => 1}).without(:b).run, {'a' => 1})
    assert_equal(r.expr({}).without(:b).run, {})
  end

  def test_arr_ordering
    assert_equal(r.lt([1,2,3],[1,2,3,4]).run, true)
    assert_equal(r.lt([1,2,3],[1,2,3]).run, false)
  end

  def test_junctions # from python tests
    assert_equal(r.any(false).run, false)
    assert_equal(r.any(true, false).run, true)
    assert_equal(r.any(false, true).run, true)
    assert_equal(r.any(false, false, true).run, true)

    assert_equal(r.all(false).run, false)
    assert_equal(r.all(true, false).run, false)
    assert_equal(r.all(true, true).run, true)
    assert_equal(r.all(true, true, true).run, true)

    assert_raise(RuntimeError){r.all(true, 3).run}
    assert_raise(RuntimeError){r.any(4, true).run}
  end

  def test_not # from python tests
    assert_equal(r.not(true).run, false)
    assert_equal(r.not(false).run, true)
    assert_raise(RuntimeError){r.not(3).run}
  end

  def test_let # from python tests
    assert_equal(r.let([["x", 3]], r.letvar("x")).run, 3)
    assert_equal(r.let([["x", 3], ["x", 4]], r.letvar("x")).run, 4)
    assert_equal(r.let([["x", 3], ["y", 4]], r.letvar("x")).run, 3)
    assert_equal(r.let([['a', 2], ['b', r.letvar('a')+1]], r.letvar('b')*2).run, 6)

    assert_equal(r.let({:x => 3}, r.letvar("x")).run, 3)
    assert_equal(r.let({:x => 3, :y => 4}, r.letvar("y")).run, 4)
  end

  def test_if # from python tests
    assert_equal(r.if(true, 3, 4).run, 3)
    assert_equal(r.if(false, 4, 5).run, 5)
    assert_equal(r.if(r.eq(3, 3), "foo", "bar").run, "foo")
    assert_raise(RuntimeError){r.if(5,1,2).run}
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
    assert_equal(r.expr([]).append(2).run.to_a, [2])
    assert_equal(r.expr([1]).append(2).run.to_a, [1, 2])
    assert_raise(RuntimeError){r.expr(2).append(0).run.to_a}

    assert_equal(r.add([1], [2]).run.to_a, [1, 2])
    assert_equal(r.add([1, 2], []).run.to_a, [1, 2])
    assert_raise(RuntimeError){r.add(1,[1]).run}
    assert_raise(RuntimeError){r.add([1],1).run}

    arr = (0...10).collect{|x| x}
    assert_equal(r.expr(arr)[0...3].run.to_a, arr[0...3])
    assert_equal(r.expr(arr)[0...0].run.to_a, arr[0...0])
    assert_equal(r.expr(arr)[5...15].run.to_a, arr[5...15])
    assert_equal(r.expr(arr)[5...-3].run.to_a, arr[5...-3])
    assert_equal(r.expr(arr)[-5...-3].run.to_a, arr[-5...-3])
    assert_equal(r.expr(arr)[0..3].run.to_a, arr[0..3])
    assert_equal(r.expr(arr)[0..0].run.to_a, arr[0..0])
    assert_equal(r.expr(arr)[5..15].run.to_a, arr[5..15])
    assert_equal(r.expr(arr)[5..-3].run.to_a, arr[5..-3])
    assert_equal(r.expr(arr)[-5..-3].run.to_a, arr[-5..-3])

    assert_raise(RuntimeError){r.expr(1)[0...0].run.to_a}
    assert_raise(RuntimeError){r.expr(arr)[0.5...0].run.to_a}
    assert_raise(RuntimeError){r.expr(1)[0...1.01].run.to_a}
    assert_raise(RuntimeError){r.expr(1)[5...3].run.to_a}

    assert_equal(r.expr(arr)[5..-1].run.to_a, arr[5..-1])
    assert_equal(r.expr(arr)[0...7].run.to_a, arr[0...7])
    assert_equal(r.expr(arr)[0...-2].run.to_a, arr[0...-2])
    assert_equal(r.expr(arr)[-2..-1].run.to_a, arr[-2..-1])
    assert_equal(r.expr(arr)[0..-1].run.to_a, arr[0..-1])

    assert_equal(r.expr(arr)[3].run, 3)
    assert_equal(r.expr(arr)[-1].run, 9)
    assert_raise(RuntimeError){r.expr(0)[0].run}
    assert_raise(SyntaxError){r.expr(arr)[0.1].run}
    assert_raise(RuntimeError){r.expr([0])[1].run}

    assert_equal(r.expr([]).length.run, 0)
    assert_equal(r.expr(arr).length.run, arr.length)
    assert_raise(RuntimeError){r.expr(0).length.run}
  end

  def test_stream # from python tests
    arr = (0...10).collect{|x| x}
    assert_equal(r.expr(arr).to_stream.to_array.run, arr)
    assert_equal(r.expr(arr).to_stream.nth(0).run, 0)
    assert_equal(r.expr(arr).to_stream.nth(5).run, 5)
    assert_raise(RuntimeError){r.expr(arr).to_stream.nth([]).run}
    assert_raise(RuntimeError){r.expr(arr).to_stream.nth(0.4).run}
    assert_raise(RuntimeError){r.expr(arr).to_stream.nth(-5).run}
    assert_raise(RuntimeError){r.expr([0]).to_stream.nth(1).run}
  end

  def test_stream_fancy # from python tests
    def limit(a,c); r.expr(a).to_stream.limit(c).to_array.run; end
    def skip(a,c); r.expr(a).to_stream.skip(c).to_array.run; end

    assert_equal(limit([], 0), [])
    assert_equal(limit([1, 2], 0), [])
    assert_equal(limit([1, 2], 1), [1])
    assert_equal(limit([1, 2], 5), [1, 2])
    assert_raise(RuntimeError){limit([], -1)}

    assert_equal(skip([], 0), [])
    assert_equal(skip([1, 2], 5), [])
    assert_equal(skip([1, 2], 0), [1, 2])
    assert_equal(skip([1, 2], 1), [2])

    def distinct(a); r.expr(a).distinct.run; end
    assert_equal(distinct([]), [])
    assert_equal(distinct([1,2,3]*10), [1,2,3])
    assert_equal(distinct([1, 2, 3, 2]), [1, 2, 3])
    assert_equal(distinct([true, 2, false, 2]), [true, 2, false])
  end

  def test_ordering
    def order(query, *args); query.orderby(*args).run.to_a; end
    docs = (0...10).map{|n| {'id' => 100+n, 'a' => n, 'b' => n%3}}
    assert_equal(order(r.expr(docs).to_stream, 'a'), docs.sort_by{|x| x['a']})
    assert_equal(order(r.expr(docs).to_stream,['a', false]), docs.sort_by{|x| x['a']}.reverse)
    assert_equal(order(r.expr(docs).to_stream.filter({'b' => 0}), :a),
                 docs.select{|x| x['b'] == 0}.sort_by{|x| x['a']})
  end

  def test_ops # +,-,%,*,/,<,>,<=,>=,eq,ne,any,all
    assert_equal((r.expr(5) + 3).run, 8)
    assert_equal((r.expr(5).add(3)).run, 8)
    assert_equal(r.add(5,3).run, 8)
    assert_equal(r.add(2,3,3).run, 8)

    assert_equal((r.expr(5) - 3).run, 2)
    assert_equal((r.expr(5).sub(3)).run, 2)
    assert_equal((r.expr(5).subtract(3)).run, 2)
    assert_equal(r.sub(5,3).run, 2)
    assert_equal(r.subtract(5,3).run, 2)

    assert_equal((r.expr(5) % 3).run, 2)
    assert_equal((r.expr(5).mod(3)).run, 2)
    assert_equal((r.expr(5).modulo(3)).run, 2)
    assert_equal(r.mod(5,3).run, 2)
    assert_equal(r.modulo(5,3).run, 2)

    assert_equal((r.expr(5) * 3).run, 15)
    assert_equal((r.expr(5).mul(3)).run, 15)
    assert_equal((r.expr(5).multiply(3)).run, 15)
    assert_equal(r.mul(5,3).run, 15)
    assert_equal(r.multiply(5,3).run, 15)
    assert_equal(r.multiply(5,3,1).run, 15)

    assert_equal((r.expr(15) / 3).run, 5)
    assert_equal((r.expr(15).div(3)).run, 5)
    assert_equal((r.expr(15).divide(3)).run, 5)
    assert_equal(r.div(15,3).run, 5)
    assert_equal(r.divide(15,3).run, 5)

    assert_equal(r.lt(3,2).run,false)
    assert_equal(r.lt(3,3).run,false)
    assert_equal(r.lt(3,4).run,true)
    assert_equal(r.expr(3).lt(2).run,false)
    assert_equal(r.expr(3).lt(3).run,false)
    assert_equal(r.expr(3).lt(4).run,true)
    assert_equal((r.expr(3) < 2).run,false)
    assert_equal((r.expr(3) < 3).run,false)
    assert_equal((r.expr(3) < 4).run,true)

    assert_equal(r.le(3,2).run,false)
    assert_equal(r.le(3,3).run,true)
    assert_equal(r.le(3,4).run,true)
    assert_equal(r.expr(3).le(2).run,false)
    assert_equal(r.expr(3).le(3).run,true)
    assert_equal(r.expr(3).le(4).run,true)
    assert_equal((r.expr(3) <= 2).run,false)
    assert_equal((r.expr(3) <= 3).run,true)
    assert_equal((r.expr(3) <= 4).run,true)

    assert_equal(r.gt(3,2).run,true)
    assert_equal(r.gt(3,3).run,false)
    assert_equal(r.gt(3,4).run,false)
    assert_equal(r.expr(3).gt(2).run,true)
    assert_equal(r.expr(3).gt(3).run,false)
    assert_equal(r.expr(3).gt(4).run,false)
    assert_equal((r.expr(3) > 2).run,true)
    assert_equal((r.expr(3) > 3).run,false)
    assert_equal((r.expr(3) > 4).run,false)

    assert_equal(r.ge(3,2).run,true)
    assert_equal(r.ge(3,3).run,true)
    assert_equal(r.ge(3,4).run,false)
    assert_equal(r.expr(3).ge(2).run,true)
    assert_equal(r.expr(3).ge(3).run,true)
    assert_equal(r.expr(3).ge(4).run,false)
    assert_equal((r.expr(3) >= 2).run,true)
    assert_equal((r.expr(3) >= 3).run,true)
    assert_equal((r.expr(3) >= 4).run,false)

    assert_equal(r.eq(3,2).run, false)
    assert_equal(r.eq(3,3).run, true)
    assert_equal(r.equals(3,2).run, false)
    assert_equal(r.equals(3,3).run, true)

    assert_equal(r.ne(3,2).run, true)
    assert_equal(r.ne(3,3).run, false)
    assert_equal(r.not(r.equals(3,2)).run, true)
    assert_equal(r.not(r.equals(3,3)).run, false)
    assert_equal(r.equals(3,2).not.run, true)
    assert_equal(r.equals(3,3).not.run, false)

    assert_equal(r.all(true, true, true).run, true)
    assert_equal(r.all(true, false, true).run, false)
    assert_equal(r.and(true, true, true).run, true)
    assert_equal(r.and(true, false, true).run, false)
    assert_equal((r.expr(true).and(true)).run, true)
    assert_equal((r.expr(true).and(false)).run, false)

    assert_equal(r.any(false, false, false).run, false)
    assert_equal(r.any(false, true, false).run, true)
    assert_equal(r.or(false, false, false).run, false)
    assert_equal(r.or(false, true, false).run, true)
    assert_equal((r.expr(false).or(false)).run, false)
    assert_equal((r.expr(true).or(false)).run, true)
  end

  def test_json # JSON
    assert_equal(r.json('[1,2,3]').run, [1,2,3])
  end

  def test_let # LET, CALL, VAR, NUMBER, STRING
    query = r.let([['a', r.add(1,2)], ['b', r.add(r.letvar('a'),2)]], r.letvar('a') + r.letvar('b'))
    assert_equal(query.run, 8)
  end

  def test_easy_read # TABLE
    assert_equal($data, id_sort(rdb.run.to_a))
    assert_equal($data, id_sort(r.db('test').table('Welcome-rdb').run.to_a))
  end

  def test_error # IF, JSON, ERROR
    query = r.if(r.add(1,2) >= 3, r.json('[1,2,3]'), r.error('unreachable'))
    query_err = r.if(r.add(1,2) > 3, r.json('[1,2,3]'), r.error('reachable'))
    assert_equal(query.run, [1,2,3])
    assert_raise(RuntimeError){assert_equal(query_err.run)}
  end

  def test_array # BOOL, JSON_NULL, ARRAY, ARRAYTOSTREAM
    assert_equal(r.expr([true, false, nil]).run, [true, false, nil])
    assert_equal(r.expr([true, false, nil]).arraytostream.run.to_a, [true, false, nil])
    assert_equal(r.expr([true, false, nil]).to_stream.run.to_a, [true, false, nil])
  end

  def test_getbykey # OBJECT, GETBYKEY
    query = r.expr({'obj' => rdb.get(0)})
    query2 = r.expr({'obj' => rdb.get(0)})
    assert_equal(query.run['obj'], $data[0])
    assert_equal(query2.run['obj'], $data[0])
  end

  def test_map # MAP, FILTER, GETATTR, IMPLICIT_GETATTR, STREAMTOARRAY
    assert_equal(r.expr([{:id => 1}, {:id => 2}]).map{|row| row[:id]}.run.to_a, [1,2])
    assert_raise(RuntimeError){r.expr(1).map{}.run.to_a}
    assert_equal(rdb.filter({'num' => 1}).run.to_a, [$data[1]])
    assert_equal(id_sort(rdb.filter({'num' => r[:num]}).run.to_a), $data)
    query = rdb.orderby(:id).map { |outer_row|
      rdb.filter{|row| row[:id] < outer_row[:id]}.to_array
    }
    assert_equal(id_sort(query.run.to_a[2]), $data[0..1])
  end

  def test_reduce # REDUCE, HASATTR, IMPLICIT_HASATTR
    # TODO: Error checking for reduce
    # TODO_SERV: Reduce still broken.
    query = rdb.map{r[:$_].hasattr(:id)}.reduce(true){|a,b| r.all a,b}
    #assert_equal(query.run, true)

    assert_equal(r.expr([1,2,3]).reduce(0){|a,b| a+b}.run, 6)
    assert_raise(RuntimeError){r.expr(1).reduce(0){0}.run}

    # assert_equal(  rdb.map{|row| row['id']}.reduce(0){|a,b| a+b}.run,
    #              $data.map{|row| row['id']}.reduce(0){|a,b| a+b})

    assert_equal(rdb.map{r[:$_].hasattr(:id)}.run.to_a, $data.map{true})
    assert_equal(rdb.map{|row| r.not row.hasattr('id')}.run.to_a, $data.map{false})
    assert_equal(rdb.map{|row| r.not row.has('id')}.run.to_a, $data.map{false})
    assert_equal(rdb.map{|row| r.not row.attr?('id')}.run.to_a, $data.map{false})
    assert_equal(rdb.map{r[:$_].has(:id)}.run.to_a, $data.map{true})
    assert_equal(rdb.map{r[:$_].attr?('id').not}.run.to_a, $data.map{false})
  end

  def test_filter # FILTER
    assert_equal(r.expr([1,2,3]).filter{|x| x<3}.run.to_a, [1,2])
    assert_raise(RuntimeError){r.expr(1).filter{true}.run.to_a}
    query_5 = rdb.filter{r[:name].eq('5')}
    assert_equal(query_5.run.to_a, [$data[5]])
    query_2345 = rdb.filter{|row| r.and row[:id] >= 2,row[:id] <= 5}
    query_234 = query_2345.filter{|row| row[:num].neq(5)}
    query_23 = query_234.filter{|row| r.any row[:num].eq(2),row[:num].equals(3)}
    assert_equal(id_sort(query_2345.run.to_a), $data[2..5])
    assert_equal(id_sort(query_234.run.to_a), $data[2..4])
    assert_equal(id_sort(query_23.run.to_a), $data[2..3])
    assert_equal(id_sort(rdb.filter(r[:id] < 5).run.to_a), $data[0...5])
  end

  def test_too_big_key
    assert_raise(RuntimeError){rdb.upsert({:id => 'a'*1000}).run}
  end

  def test_implicit_nesting
    assert_raise(SyntaxError){rdb.filter{rdb.filter{r[:id]}.to_array.eq([])}.run.to_a}
  end

  def test_slice_streams # SLICE
    arr=[0,1,2,3,4,5]
    assert_equal(r.expr(arr)[1].run, 1)
    assert_equal(r.expr(arr).to_stream[1].run, 1)

    assert_equal(r.expr(arr)[2...6].run.to_a, r.expr(arr)[2..-1].run.to_a)
    assert_equal(r.expr(arr).to_stream[2...6].run.to_a, r.expr(arr).to_stream[2..-1].run.to_a)

    assert_equal(r.expr(arr)[2...5].run.to_a, r.expr(arr)[2..4].run.to_a)
    assert_equal(r.expr(arr).to_stream[2...5].run.to_a, r.expr(arr).to_stream[2..4].run.to_a)

    assert_raise(RuntimeError){r.expr(arr)[2...0].run.to_a}
    assert_raise(RuntimeError){r.expr(arr).to_stream[2...-1].run.to_a}
  end

  def test_mapmerge
    assert_equal(r.expr({:a => 1}).mapmerge({:b => 2}).run, {'a' => 1, 'b' => 2})
    assert_equal(r.mapmerge({:a => 1}, {:a => 2}).run, {'a' => 2})
  end

  def test_orderby # ORDERBY, MAP
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    assert_equal(rdb.orderby('id').run.to_a, $data)
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    assert_equal(rdb.orderby([:id, false]).run.to_a, $data.reverse)
    query = rdb.map{r.expr({:id => r[:id],:num => r[:id].mod(2)})}.orderby(:num,[:id, false])
    want = $data.map{|o| o['id']}.sort_by{|n| (n%2)*$data.length - n}
    assert_equal(query.run.to_a.map{|o| o['id']}, want)
  end

  def test_concatmap # CONCATMAP, DISTINCT
    assert_equal(r.expr([1,2,3]).concatmap{r.expr([1,2]).to_stream}.to_stream.run.to_a,
                 [1,2,1,2,1,2])
    assert_equal(r.expr([[1],[2]]).concatmap{|x| x}.run.to_a, [1,2])
    assert_raise(RuntimeError){r.expr([[1],2]).concatmap{|x| x}.run.to_a}
    assert_raise(RuntimeError){r.expr(1).concatmap{|x| x}.run.to_a}
    assert_equal(r.expr([[1],[2]]).concatmap{|x| x}.run.to_a, [1,2])
    query = rdb.concatmap{|row| rdb.map{r[:id] * row[:id]}}.distinct
    nums = $data.map{|o| o['id']}
    want = nums.map{|n| nums.map{|m| n*m}}.flatten(1).uniq
    assert_equal(query.run.to_a.sort, want.sort)
  end

  def test_range # RANGE
    assert_raise(RuntimeError){rdb.between(1, r.expr([3])).run.to_a}
    assert_equal(id_sort(rdb.between(1,3).run.to_a), $data[1..3])
    assert_equal(id_sort(rdb.between(2,nil).run.to_a), $data[2..-1])
    assert_equal(id_sort(rdb.between(1, 3).run.to_a), $data[1..3])
    assert_equal(id_sort(rdb.between(nil, 4).run.to_a),$data[0..4])

    assert_equal(id_sort(rdb.to_array.between(1,3).run.to_a), $data[1..3])
    assert_equal(id_sort(rdb.to_array.between(2,nil).run.to_a), $data[2..-1])
    assert_equal(id_sort(rdb.to_array.between(1, 3).run.to_a), $data[1..3])
    assert_equal(id_sort(rdb.to_array.between(nil, 4).run.to_a),$data[0..4])

    assert_equal(id_sort(r.expr($data).between(1,3).run.to_a), $data[1..3])
    assert_equal(id_sort(r.expr($data).between(2,nil).run.to_a), $data[2..-1])
    assert_equal(id_sort(r.expr($data).between(1, 3).run.to_a), $data[1..3])
    assert_equal(id_sort(r.expr($data).between(nil, 4).run.to_a),$data[0..4])

    assert_raise(RuntimeError){r.expr([1]).between(1, 3).run.to_a}
    assert_raise(RuntimeError){r.expr([1,2]).between(1, 3).run.to_a}
  end

  def test_groupedmapreduce # GROUPEDMAPREDUCE
    #TODO: Add tests once issue #922 is resolved.
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    assert_equal(rdb.to_array.orderby(:id).run.to_a, $data)
    assert_equal(r.expr([{:id => 1}, {:id => 0}]).orderby(:id).run.to_a.map{|x| x['id']}, [0, 1])
    assert_raise(RuntimeError){r.expr(1).orderby(:id).run.to_a}
    assert_raise(RuntimeError){r.expr([1]).nth(0).orderby(:id).run.to_a}
    assert_raise(RuntimeError){r.expr([1]).orderby(:id).run.to_a}
    assert_raise(RuntimeError){r.expr([{:num => 1}]).orderby(:id).run.to_a}
    assert_equal(r.expr([]).orderby(:id).run.to_a, [])

    gmr = rdb.groupedmapreduce(lambda {|row| row[:id] % 4}, lambda {|row| row[:id]},
                               0, lambda {|a,b| a+b});
    gmr2 = rdb.groupedmapreduce(lambda {r[:id] % 4}, lambda {r[:id]},
                                0, lambda {|a,b| a+b});
    gmr3 = rdb.to_array.groupedmapreduce(lambda {|row| row[:id] % 4},
                                         lambda {|row| row[:id]},
                                         0, lambda {|a,b| a+b});
    gmr4 = r.expr($data).groupedmapreduce(lambda {|row| row[:id] % 4},
                                     lambda {|row| row[:id]},
                                     0, lambda {|a,b| a+b});
    assert_equal(gmr.run.to_a, gmr2.run.to_a)
    assert_equal(gmr.run.to_a, gmr3.run.to_a)
    assert_equal(gmr.run.to_a, gmr4.run.to_a)
    gmr5 = r.expr([$data]).groupedmapreduce(lambda {|row| row[:id] % 4},
                                       lambda {|row| row[:id]},
                                       0, lambda {|a,b| a+b});
    gmr6 = r.expr(1).groupedmapreduce(lambda {|row| row[:id] % 4},
                                 lambda {|row| row[:id]},
                                 0, lambda {|a,b| a+b});
    assert_raise(RuntimeError){gmr5.run.to_a}
    assert_raise(RuntimeError){gmr6.run.to_a}
    gmr.run.to_a.each {|obj|
      want = $data.map{|x| x['id']}.select{|x| x%4 == obj['group']}.reduce(0, :+)
      assert_equal(obj['reduction'], want)
    }
    assert_equal(rdb.orderby(:id).run.to_a, $data)
  end

  def test_nth # NTH
    assert_equal(rdb.orderby(:id).nth(2).run, $data[2])
  end

  def test_javascript # JAVASCRIPT
    assert_equal(r.javascript('1').run, 1)
    assert_equal(r.js('1').run, 1)
    assert_equal(r.js('1').add(r.js('2')).run, 3)
    assert_equal(r.js('2+2').run, 4)
    assert_equal(r.js('"cows"').run, "cows")
    assert_equal(r.js('[1,2,3]').run, [1,2,3])
    assert_equal(r.js('{}').run, {})
    assert_equal(r.js('{a: "whee"}').run, {"a" => "whee"})
    assert_equal(r.js('this').run, {})

    assert_equal(r.js('return 0;', :func).run, 0)
    assert_raise(RuntimeError){r.js('undefined').run}
    assert_raise(RuntimeError){r.js(body='return;').run}
    assert_raise(RuntimeError){r.js(body='var x = {}; x.x = x; return x;').run}
  end

  def test_javascript_vars # JAVASCRIPT
    assert_equal(r.let([['x', 2]], r.js('x')).run, 2)
    assert_equal(r.let([['x', 2], ['y', 3]], r.js('x+y')).run, 5)
    assert_equal(id_sort(rdb.map{|x| r.js("#{x}")}.run.to_a), id_sort(rdb.run.to_a))
    assert_equal(id_sort(rdb.map{ r.js("this")}.run.to_a), id_sort(rdb.run.to_a))
    assert_equal(rdb.map{|x| r.js("#{x}.num")}.run.to_a.sort, rdb.map{r[:num]}.run.to_a.sort)
    assert_equal(rdb.map{ r.js("this.num")}.run.to_a.sort, rdb.map{r[:num]}.run.to_a.sort)
    assert_equal(id_sort(rdb.filter{|x| r.js("#{x}.id < 5")}.run.to_a),
                 id_sort(rdb.filter{r[:id] < 5}.run.to_a))
    assert_equal(id_sort(rdb.filter{ r.js("this.id < 5")}.run.to_a),
                 id_sort(rdb.filter{r[:id] < 5}.run.to_a))
  end

  def test_pluck
    e=r.expr([{ 'a' => 1, 'b' => 2, 'c' => 3},
              { 'a' => 4, 'b' => 5, 'c' => 6}])
    assert_equal(e.pluck().run(), [{}, {}])
    assert_equal(e.pluck('a').run(), [{ 'a' => 1 }, { 'a' => 4 }])
    assert_equal(e.pluck('a', 'b').run(), [{ 'a' => 1, 'b' => 2 }, { 'a' => 4, 'b' => 5 }])
  end

  def test_pickattrs # PICKATTRS, # UNION, # LENGTH
    q1=r.union(rdb.map{r[:$_].pickattrs(:id,:name)}, rdb.map{|row| row.attrs(:id,:num)})
    q2=r.union(rdb.map{r[:$_].attrs(:id,:name)}, rdb.map{|row| row.pick(:id,:num)})
    q1v = q1.run.to_a.sort_by{|x| x['name'].to_s + ',' + x['id'].to_s}
    assert_equal(q1v, q2.run.to_a.sort_by{|x| x['name'].to_s + ',' + x['id'].to_s})
    len = $data.length
    assert_equal(q2.length.run, 2*len)
    assert_equal(q1v.map{|o| o['num']}[len..2*len-1], Array.new(len,nil))
    assert_equal(q1v.map{|o| o['name']}[0..len-1], Array.new(len,nil))
  end

  def test_pointdelete
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    assert_equal(rdb.get(0).delete.run, {'deleted' => 1})
    assert_equal(rdb.get(0).delete.run, {'deleted' => 0})
    assert_equal(rdb.orderby(:id).run.to_a, $data[1..-1])
    assert_equal(rdb.upsert($data[0]).run,
                 {'inserted' => 1, 'errors' => 0, 'generated_keys' => []})
    assert_equal(rdb.orderby(:id).run.to_a, $data)
  end

  def test___setup
    begin
      r.db('test').drop_table('Welcome-rdb').run()
    rescue
      # It don't matter to Jesus
    end
    r.db('test').create_table('Welcome-rdb').run()
  end

  def test__setup
    rdb.delete.run
    rdb.upsert($data).run
  end

  def test___write_python # from python tests
    rdb.delete.run
    docs = [{"a" => 3, "b" => 10, "id" => 1},
            {"a" => 9, "b" => -5, "id" => 2},
            {"a" => 9, "b" => 3, "id" => 3}]
    assert_equal(rdb.upsert(docs).run,
                 {'inserted' => docs.length, 'errors' => 0, 'generated_keys' => []})
    docs.each {|doc| assert_equal(rdb.get(doc['id']).run, doc)}
    assert_equal(rdb.distinct(:a).run.to_a.sort, [3,9].sort)
    assert_equal(rdb.filter({'a' => 3}).run.to_a, [docs[0]])
    assert_raise(RuntimeError){rdb.filter({'a' => rdb.length + ""}).run.to_a}
    assert_raise(RuntimeError){rdb.upsert({'a' => 3}).run.to_a}

    assert_equal(rdb.get(0).run, nil)

    assert_equal(rdb.upsert({:id => 100, :text => "グルメ"}).run,
                 {'inserted' => 1, 'errors' => 0, 'generated_keys' => []})
    assert_equal(rdb.get(100)[:text].run, "グルメ")

    rdb.delete.run
    docs = [{"id" => 1}, {"id" => 2}]
    assert_equal(rdb.upsert(docs).run,
                 {'inserted' => docs.length, 'errors' => 0, 'generated_keys' => []})
    assert_equal(rdb.limit(1).delete.run, {'deleted' => 1})
    assert_equal(rdb.run.to_a.length, 1)

    rdb.delete.run
    docs = (0...4).map{|n| {"id" => 100 + n, "a" => n, "b" => n % 3}}
    assert_equal(rdb.upsert(docs).run,
                 {'inserted' => docs.length, 'errors' => 0, 'generated_keys' => []})
    assert_equal(rdb.upsert(r.expr(docs).to_stream).run,
                 {'inserted' => docs.length, 'errors' => 0, 'generated_keys' => []})
    docs.each{|doc| assert_equal(rdb.get(doc['id']).run, doc)}

    rdb.delete.run
    docs = (0...10).map{|n| {"id" => 100 + n, "a" => n, "b" => n % 3}}
    assert_equal(rdb.upsert(docs).run,
                 {'inserted' => docs.length, 'errors' => 0, 'generated_keys' => []})
    def filt(expr, fn)
      assert_equal(rdb.filter(exp).orderby(:id).run.to_a, docs.select{|x| fn x})
    end

    # TODO: still an error?
    # assert_raise(SyntaxError){r[:a] == 5}
    # assert_raise(SyntaxError){r[:a] != 5}
    assert_equal(id_sort(rdb.filter{r[:a] < 5}.run.to_a), docs.select{|x| x['a'] < 5})
    assert_equal(id_sort(rdb.filter{r[:a] <= 5}.run.to_a), docs.select{|x| x['a'] <= 5})
    assert_equal(id_sort(rdb.filter{r[:a] > 5}.run.to_a), docs.select{|x| x['a'] > 5})
    assert_equal(id_sort(rdb.filter{r[:a] >= 5}.run.to_a), docs.select{|x| x['a'] >= 5})
    assert_raise(ArgumentError){5 < r[:a]}
    assert_raise(ArgumentError){5 <= r[:a]}
    assert_raise(ArgumentError){5 > r[:a]}
    assert_raise(ArgumentError){5 >= r[:a]}
    assert_equal(id_sort(rdb.filter{r[:a].eq(r[:b])}.run.to_a),
                 docs.select{|x| x['a'] == x['b']})

    assert_equal(-r.expr(5).run, r.expr(-5).run)
    assert_equal(+r.expr(5).run, r.expr(+5).run)

    assert_equal((r.expr(3)+4).run, 7)
    assert_equal((r.expr(3)-4).run, -1)
    assert_equal((r.expr(3)*4).run, 12)
    assert_equal((r.expr(3)/4).run, 3.0/4)
    assert_equal((r.expr(3)%2).run, 3%2)
    assert_raise(TypeError){4+r.expr(3)}
    assert_raise(TypeError){4-r.expr(3)}
    assert_raise(TypeError){4*r.expr(3)}
    assert_raise(TypeError){4/r.expr(3)}
    assert_raise(TypeError){4%r.expr(3)}

    assert_equal(((r.expr(3)+4)*-r.expr(6)*(r.expr(-5)+3)).run, 84)

    rdb.delete.run
    docs = (0...10).map{|n| {"id" => 100 + n, "a" => n, "b" => n % 3}}
    assert_equal(rdb.upsert(docs).run,
                 {'inserted' => docs.length, 'errors' => 0, 'generated_keys' => []})
    assert_equal(rdb.mutate{|x| x}.run,
                 {'modified' => docs.length, 'inserted' => 0,
                   'deleted' => 0, 'errors' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb.run.to_a), docs)
    assert_equal(rdb.update{nil}.run, {'updated' => 0, 'skipped' => 10, 'errors' => 0})

    res = rdb.update{1}.run
    assert_equal(res['errors'], 10)
    res = rdb.update{|row| row[:id]}.run
    assert_equal(res['errors'], 10)
    res = rdb.update{r[:id]}.run
    assert_equal(res['errors'], 10)

    res = rdb.update{|row| r.if((r[:id]%2).eq(0), {:id => -1}, row)}.run;
    assert_not_nil(res['first_error'])
    filtered_res = res.reject {|k,v| k == 'first_error'}
    assert_not_equal(filtered_res, res)
    assert_equal(filtered_res, {'updated' => 5, 'skipped' => 0, 'errors' => 5})

    assert_equal(id_sort(rdb.run.to_a), docs);
  end

  def test_getitem #from python tests
    arr = (0...10).map{|x| x}
    [0..-1, 2..-1, 0...2, -1..-1, 0...-1, 3...5, 3, -1].each {|rng|
      assert_equal(r.expr(arr)[rng].run, arr[rng])}
    obj = {:a => 3, :b => 4}
    [:a, :b].each {|attr|
      assert_equal(r.expr(obj)[attr].run, obj[attr])}
    assert_raise(RuntimeError){r.expr(obj)[:c].run}
  end

  def test_stream_getitem #from python tests
    arr = (0...10).map{|x| x}
    [0..-1, 3..-1, 0...3, 4...6].each {|rng|
      assert_equal(r.expr(arr).to_stream[rng].run.to_a, arr[rng])}
    [3].each {|rng|
      assert_equal(r.expr(arr).to_stream[rng].run, arr[rng])}
    assert_raise(ArgumentError){r.expr(arr).to_stream[4...'a'].run.to_a}
  end

  def test_array_foreach #FOREACH
    assert_equal(id_sort(rdb.run.to_a), $data)
    assert_equal(r.expr([2,3,4]).foreach{|x|
                   [rdb.get(x).update({:num => 0}),
                    rdb.get(x*2).update({:num => 0})]}.run,
                 {"skipped"=>0, "updated"=>6, "errors"=>0})
    assert_equal(rdb.to_array.foreach{|row|
                   rdb.filter{r[:id].eq(row[:id])}.update{|row|
                     r.if(row[:num].eq(0), {:num => row[:id]}, nil)}}.run,
                 {"skipped"=>4, "updated"=>6, "errors"=>0})
    assert_equal(id_sort(rdb.run.to_a), $data)
  end

  def test_foreach_multi #FOREACH
    db_name = "foreach_multi" + rand().to_s
    table_name = "foreach_multi_tbl" + rand().to_s
    table_name_2 = table_name + rand().to_s
    table_name_3 = table_name_2 + rand().to_s
    table_name_4 = table_name_3 + rand().to_s

    assert_equal(r.create_db(db_name).run, nil)
    assert_equal(r.db(db_name).create_table(table_name).run, nil)
    assert_equal(r.db(db_name).create_table(table_name_2).run, nil)
    assert_equal(r.db(db_name).create_table(table_name_3).run, nil)
    assert_equal(r.db(db_name).create_table(table_name_4).run, nil)
    rdb = r.db(db_name).table(table_name)
    rdb2 = r.db(db_name).table(table_name_2)
    rdb3 = r.db(db_name).table(table_name_3)
    rdb4 = r.db(db_name).table(table_name_4)

    assert_equal(rdb.upsert($data).run,
                 {'inserted' => 10, 'errors' => 0, 'generated_keys' => []})
    assert_equal(rdb2.upsert($data).run,
                 {'inserted' => 10, 'errors' => 0, 'generated_keys' => []})

    query = rdb.foreach {|row|
      rdb2.foreach {|row2|
        rdb3.upsert({'id' => row[:id]*1000 + row2[:id]})
      }
    }
    assert_equal(query.run,
                 {'inserted' => 100, 'errors' => 0, 'generated_keys' => []})

    query = rdb.foreach {|row|
      rdb2.filter{r[:id].eq(row[:id] * row[:id])}.delete
    }
    assert_equal(query.run, {'deleted' => 4})

    query = rdb.foreach {|row|
      rdb2.foreach {|row2|
        rdb3.filter{r[:id].eq(row[:id]*1000 + row2[:id])}.delete
      }
    }
    assert_equal(query.run, {'deleted' => 60})

    assert_equal(rdb.foreach{|row| rdb4.upsert(row)}.run,
                 {'inserted' => 10, 'errors' => 0, 'generated_keys' => []})
    assert_equal(rdb2.foreach{|row| rdb4.filter{r[:id].eq(row[:id])}.delete}.run,
                 {'deleted' => 6})

    # TODO: Pending resolution of issue #930
    # assert_equal(rdb3.orderby(:id).run,
    #              r.let([['x', rdb2.concatmap{|row|
    #                        rdb.filter{
    #                          r[:id].neq(row[:id])
    #                        }
    #                      }.map{r[:id]}.distinct.to_array]],
    #                    rdb.concatmap{|row|
    #                      r[:$x].to_stream.map{|id| row[:id]*1000+id}}).run)
  end

  def test_fancy_foreach #FOREACH
    db_name = rand().to_s
    table_name = rand().to_s
    assert_equal(r.create_db(db_name).run, nil)
    assert_equal(r.db(db_name).create_table(table_name).run, nil)
    rdb = r.db(db_name).table(table_name)

    assert_equal(rdb.upsert($data).run,
                 {'inserted' => 10, 'errors' => 0, 'generated_keys' => []})
    rdb.upsert({:id => 11, :broken => true}).run
    rdb.upsert({:id => 12, :broken => true}).run
    rdb.upsert({:id => 13, :broken => true}).run
    query = rdb.orderby(:id).foreach { |row|
      [rdb.update { |row2|
         r.if(row[:id].eq(row2[:id]),
              {:num => row2[:num]*2},
              nil)
       },
       rdb.filter{r[:id].eq(row[:id])}.mutate{ |row|
         r.if((r[:id]%2).eq(1), nil, row)
       },
       rdb.get(0).delete,
       rdb.get(12).delete]
    }
    res = query.run
    assert_equal(res['errors'], 2)
    assert_not_nil(res['first_error'])
    assert_equal(res['deleted'], 9)
    assert_equal(res['updated'], 10)
    assert_equal(res['modified'], 5)
    assert_equal(rdb.orderby(:id).run.to_a,
                 [{"num"=>4,  "id"=>2, "name"=>"2"},
                  {"num"=>8,  "id"=>4, "name"=>"4"},
                  {"num"=>12, "id"=>6, "name"=>"6"},
                  {"num"=>16, "id"=>8, "name"=>"8"}])
  end

  def test_bad_primary_key_type
    assert_raise(RuntimeError){rdb.upsert({:id => []}).run}
    assert_raise(RuntimeError){rdb.get(100).mutate{{:id => []}}.run}
  end

  def test_big_between
    data = (0...100).map{|x| {:id => x}}
    assert_equal(rdb.upsert(data).run,
                 {'inserted' => 100, 'errors' => 0, 'generated_keys' => []})
    assert_equal(id_sort(rdb.between(1, 2).run.to_a), [{'id' => 1}, {'id' => 2}])
    assert_equal(rdb.delete.run, {'deleted' => 100})
    assert_equal(rdb.upsert($data).run,
                 {'inserted' => 10, 'errors' => 0, 'generated_keys' => []})
    assert_equal(id_sort(rdb.run.to_a), $data)
  end

  def test_mutate_edge_cases #POINTMUTATE
    res = rdb.mutate{1}.run
    assert_equal(res['errors'], 10)
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    assert_equal(rdb.get(0).mutate{nil}.run,
                 {'deleted' => 1, 'inserted' => 0, 'modified' => 0, 'errors' => 0, 'errors' => 0})
    assert_equal(rdb.orderby(:id).run.to_a, $data[1..-1])
    assert_equal(rdb.get(0).mutate{nil}.run,
                 {'deleted' => 0, 'inserted' => 0, 'modified' => 0, 'errors' => 0, 'errors' => 0})
    assert_equal(rdb.orderby(:id).run.to_a, $data[1..-1])
    #assert_equal(rdb.get(0).mutate{|row| r.if(row.eq(nil), $data[0], $data[1])}.run,
    assert_equal(rdb.get(0).mutate{$data[0]}.run,
                 {'deleted' => 0, 'inserted' => 1, 'modified' => 0, 'errors' => 0, 'errors' => 0})
    assert_raise(RuntimeError){rdb.get(-1).mutate{{:id => []}}.run}
    assert_raise(RuntimeError){rdb.get(-1).mutate{{:id => 0}}.run}
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    assert_raise(RuntimeError) {
      rdb.get(0).mutate{|row| r.if(row.eq(nil), $data[0], $data[1])}.run.to_a
    }
    assert_equal(rdb.get(0).mutate{|row| r.if(row.eq(nil), $data[1], $data[0])}.run,
                 {'deleted' => 0, 'inserted' => 0, 'modified' => 1, 'errors' => 0, 'errors' => 0})
    assert_equal(rdb.orderby(:id).run.to_a, $data)
  end

  def test_insert
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    data2 = $data + $data.map{|x| {'id' => x['id'] + $data.length}}

    res = rdb.insert(data2).run
    assert_equal(res['inserted'], $data.length)
    assert_equal(res['errors'], $data.length)
    assert_not_nil(res['first_error'])

    assert_equal(rdb.orderby(:id).run.to_a, data2)
    assert_equal(rdb.delete.run, {'deleted' => data2.length})

    res = rdb.insert($data).run
    assert_equal(res['inserted'], $data.length)
    assert_equal(res['errors'], 0)
    assert_nil(res['first_error'])

    assert_equal(rdb.orderby(:id).run.to_a, $data)
  end

  def test_update_edge_cases #POINTUPDATE
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    assert_equal(rdb.get(0).update(nil).run,
                 {'skipped' => 1, 'updated' => 0, 'errors' => 0})
    assert_equal(rdb.get(11).update(nil).run,
                 {'skipped' => 1, 'updated' => 0, 'errors' => 0})
    assert_equal(rdb.get(11).update({}).run,
                 {'skipped' => 1, 'updated' => 0, 'errors' => 0})
    assert_equal(rdb.orderby(:id).run.to_a, $data)
  end

  def test_foreach_error #FOREACH
    assert_equal(rdb.orderby(:id).run.to_a, $data)
    query = rdb.foreach{|row| rdb.get(row[:id]).update{{:id => 0}}}
    res = query.run
    assert_equal(res['errors'], 9)
    assert_not_nil(res['first_error'])
    assert_equal(rdb.orderby(:id).run.to_a, $data)
  end

  def test___write #three underscores so it runs first
    db_name = rand().to_s
    table_name = rand().to_s

    orig_dbs = r.list_dbs.run
    assert_equal(r.create_db(db_name).run, nil)
    assert_raise(RuntimeError){r.create_db(db_name).run}
    new_dbs = r.list_dbs.run
    assert_equal(new_dbs.length, orig_dbs.length+1)

    assert_equal(r.db(db_name).create_table(table_name).run, nil)
    assert_raise(RuntimeError){r.db(db_name).create_table(table_name).run}
    assert_equal(r.db(db_name).list_tables.run, [table_name])

    assert_equal(r.db(db_name).table(table_name).upsert({:id => 0}).run,
                 {'inserted' => 1, 'errors' => 0, 'generated_keys' => []})
    assert_equal(r.db(db_name).table(table_name).run.to_a, [{'id' => 0}])
    assert_equal(r.db(db_name).create_table(table_name+table_name).run, nil)
    assert_equal(r.db(db_name).list_tables.run.length, 2)

    assert_raise(RuntimeError){r.db('').table('').run.to_a}
    assert_raise(RuntimeError){r.db('test').table(table_name).run.to_a}
    assert_raise(RuntimeError){r.db(db_name).table('').run.to_a}

    assert_equal(r.db(db_name).drop_table(table_name+table_name).run, nil)
    assert_raise(RuntimeError){r.db(db_name).table(table_name+table_name).run.to_a}
    assert_equal(r.drop_db(db_name).run, nil)
    assert_equal(r.list_dbs.run.sort, orig_dbs.sort)
    assert_raise(RuntimeError){r.db(db_name).table(table_name).run.to_a}

    assert_equal(r.create_db(db_name).run, nil)
    assert_raise(RuntimeError){r.db(db_name).table(table_name).run.to_a}
    assert_equal(r.db(db_name).create_table(table_name).run, nil)
    assert_equal(r.db(db_name).table(table_name).run.to_a, [])
    rdb2 = r.db(db_name).table(table_name)

    $data = []
    len = 10
    Array.new(len).each_index{|i| $data << {'id'=>i,'num'=>i,'name'=>i.to_s}}

    # UPSERT, UPDATE
    assert_equal(rdb2.upsert($data).run['inserted'], len)
    assert_equal(rdb2.upsert($data + $data).run['inserted'], len*2)
    assert_equal(id_sort(rdb2.run.to_a), $data)
    assert_equal(rdb2.upsert({:id => 0, :broken => true, 'errors' => 0}).run['inserted'], 1)
    assert_equal(rdb2.upsert({:id => 1, :broken => true, 'errors' => 0}).run['inserted'], 1)
    assert_equal(id_sort(rdb2.run.to_a)[2..len-1], $data[2..len-1])
    assert_equal(id_sort(rdb2.run.to_a)[0]['broken'], true)
    assert_equal(id_sort(rdb2.run.to_a)[1]['broken'], true)

    mutate = rdb2.filter{r[:id].eq(0)}.mutate{$data[0]}.run
    assert_equal(mutate, {"modified"=>1, 'inserted' => 0, "deleted"=>0, 'errors' => 0, 'errors' => 0})
    update = rdb2.update{|x| r.if(x.attr?(:broken), $data[1], x)}.run
    assert_equal(update, {'errors' => 0, 'updated' => len, 'skipped' => 0})
    mutate = rdb2.mutate{|x| {:id => x[:id], :num => x[:num], :name => x[:name]}}.run
    assert_equal(mutate, {"modified"=>10, 'inserted' => 0, "deleted"=>0, 'errors' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run.to_a), $data)

    mutate = rdb2.mutate{|row| r.if(row[:id].eq(4) | row[:id].eq(5),
                                    {:id => -1}, row)}.run
    assert_not_nil(mutate['first_error'])
    filtered_mutate = mutate.reject {|k,v| k == 'first_error'}
    assert_equal(filtered_mutate,
                 {'modified' => 8, 'inserted' => 0, 'deleted' => 0, 'errors' => 2})
    assert_equal(rdb2.get(5).update{{:num => "5"}}.run,
                 {'updated' => 1, 'skipped' => 0, 'errors' => 0})
    mutate = rdb2.mutate{|row| r.mapmerge(row, {:num => row[:num]*2})}.run
    assert_not_nil(mutate['first_error'])
    filtered_mutate = mutate.reject {|k,v| k == 'first_error'}
    assert_equal(filtered_mutate,
                 {'modified' => 9, 'inserted' => 0, 'deleted' => 0, 'errors' => 1})
    assert_equal(rdb2.get(5).run['num'], '5')
    assert_equal(rdb2.mutate{|row|
                   r.if(row[:id].eq(5),
                        $data[5],
                        row.mapmerge({:num => row[:num]/2}))}.run,
                 {'modified' => 10, 'inserted' => 0, 'deleted' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run.to_a), $data);

    update = rdb2.get(0).update({:num => 2}).run
    assert_equal(update, {'errors' => 0, 'updated' => 1, 'skipped' => 0})
    assert_equal(rdb2.get(0).run['num'], 2);
    update = rdb2.get(0).update(nil).run
    assert_equal(update, {'errors' => 0, 'updated' => 0, 'skipped' => 1})
    assert_equal(rdb2.get(0).mutate($data[0]).run,
                 {"errors"=>0, 'inserted' => 0, "deleted"=>0, "modified"=>1, 'errors' => 0})
    assert_equal(id_sort(rdb2.run.to_a), $data)

    # DELETE
    assert_equal(rdb2.filter{r[:id] < 5}.delete.run['deleted'], 5)
    assert_equal(id_sort(rdb2.run.to_a), $data[5..-1])
    assert_equal(rdb2.delete.run['deleted'], len-5)
    assert_equal(rdb2.run.to_a, [])

    # UPSERTSTREAM
    assert_equal(rdb2.upsert(r.expr($data).to_stream).run['inserted'], len)
    rdb2.upsert($data).run
    assert_equal(id_sort(rdb2.run.to_a), $data)

    # MUTATE
    assert_equal(rdb2.mutate{|row| row}.run,
                 {'modified' => len, 'inserted' => 0, 'deleted' => 0, 'errors' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run.to_a), $data)
    assert_equal(rdb2.mutate{|row| r.if(row[:id] < 5, nil, row)}.run,
                 {'modified' => 5, 'inserted' => 0, 'deleted' => len-5, 'errors' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run.to_a), $data[5..-1])
    assert_equal(rdb2.upsert($data[0...5]).run,
                 {'inserted' => 5, 'errors' => 0, 'generated_keys' => []})

    # FOREACH, POINTDELETE
    # TODO_SERV: Return value of foreach should change (Issue #874)
    query = rdb2.foreach{|row| [rdb2.get(row[:id]).delete, rdb2.upsert(row)]}
    assert_equal(query.run,
                 {'deleted' => 10, 'inserted' => 10,
                   'errors' => 0, 'generated_keys' => []})
    assert_equal(id_sort(rdb2.run.to_a), $data)
    rdb2.foreach{|row| rdb2.get(row[:id]).delete}.run
    assert_equal(id_sort(rdb2.run.to_a), [])

    rdb2.upsert($data).run
    assert_equal(id_sort(rdb2.run), $data)
    query = rdb2.get(0).update{{:id => 0, :broken => 5}}
    assert_equal(query.run, {'updated'=>1,'errors'=>0,'skipped'=>0})
    query = rdb2.upsert($data[0])
    assert_equal(query.run, {'inserted'=>1, 'errors' => 0, 'generated_keys' => []})
    assert_equal(id_sort(rdb2.run.to_a), $data)

    assert_equal(rdb2.get(0).mutate(nil).run,
                 {'modified' => 0, 'inserted' => 0, 'deleted' => 1, 'errors' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run.to_a), $data[1..-1])
    assert_raise(RuntimeError){rdb2.get(1).mutate{{:id => -1}}.run.to_a}
    assert_raise(RuntimeError){rdb2.get(1).mutate{|row| {:name => row[:name]*2}}.run.to_a}
    assert_equal(rdb2.get(1).mutate{|row| row.mapmerge({:num => 2})}.run,
                 {'modified' => 1, 'inserted' => 0, 'deleted' => 0, 'errors' => 0, 'errors' => 0})
    assert_equal(rdb2.get(1).run['num'], 2);
    assert_equal(rdb2.upsert($data[0...2]).run,
                 {'inserted' => 2, 'errors' => 0, 'generated_keys' => []})

    assert_equal(id_sort(rdb2.run.to_a), $data)
  end
end
