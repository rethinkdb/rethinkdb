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

require 'rethinkdb_shortcuts.rb'
require 'test/unit'
$port_base = ARGV[0].to_i # 0 if none given
class ClientTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts_Mixin
  def rdb; r.db('Welcome-db').table('Welcome-rdb'); end
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

    assert_equal(r.lt(false, true, 1, "", []).run, true)
    assert_equal(r.gt([], "", 1, true, false).run, true)
    assert_equal(r.lt(false, true, "", 1, []).run, false)
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
    assert_raise(RuntimeError){r.any(true, 4).run}
  end

  def test_not # from python tests
    assert_equal(r.not(true).run, false)
    assert_equal(r.not(false).run, true)
    assert_raise(RuntimeError){r.not(3).run}
  end

  def test_let # from python tests
    assert_equal(r.let([["x", 3]], r.var("x")).run, 3)
    assert_equal(r.let([["x", 3], ["x", 4]], r.var("x")).run, 4)
    assert_equal(r.let([["x", 3], ["y", 4]], r.var("x")).run, 3)
    assert_equal(r.let([['a', 2], ['b', r[:$a]+1]], r[:$b]*2).run, 6)

    assert_equal(r.let({:x => 3}, r.var("x")).run, 3)
    assert_equal(r.let({:x => 3, :y => 4}, r.var("y")).run, 4)
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
    assert_equal(r[[]].append(2).run, [2])
    assert_equal(r[[1]].append(2).run, [1, 2])
    assert_raise(RuntimeError){r[2].append(0).run}

    assert_equal(r.add([1], [2]).run, [1, 2])
    assert_equal(r.add([1, 2], []).run, [1, 2])
    assert_raise(RuntimeError){r.add(1,[1]).run}
    assert_raise(RuntimeError){r.add([1],1).run}

    arr = (0...10).collect{|x| x}
    assert_equal(r[arr][0...3].run, arr[0...3])
    assert_equal(r[arr][0...0].run, arr[0...0])
    assert_equal(r[arr][5...15].run, arr[5...15])
    assert_equal(r[arr][5...-3].run, arr[5...-3])
    assert_equal(r[arr][-5...-3].run, arr[-5...-3])
    assert_equal(r[arr][0..3].run, arr[0..3])
    assert_equal(r[arr][0..0].run, arr[0..0])
    assert_equal(r[arr][5..15].run, arr[5..15])
    assert_equal(r[arr][5..-3].run, arr[5..-3])
    assert_equal(r[arr][-5..-3].run, arr[-5..-3])

    assert_raise(RuntimeError){r[1][0...0].run}
    assert_raise(RuntimeError){r[arr][0.5...0].run}
    assert_raise(RuntimeError){r[1][0...1.01].run}
    assert_raise(RuntimeError){r[1][5...3].run}

    assert_equal(r[arr][5..-1].run, arr[5..-1])
    assert_equal(r[arr][0...7].run, arr[0...7])
    assert_equal(r[arr][0...-2].run, arr[0...-2])
    assert_equal(r[arr][-2..-1].run, arr[-2..-1])
    assert_equal(r[arr][0..-1].run, arr[0..-1])

    assert_equal(r[arr][3].run, 3)
    assert_equal(r[arr][-1].run, 9)
    assert_raise(RuntimeError){r[0][0].run}
    assert_raise(SyntaxError){r[arr][0.1].run}
    assert_raise(RuntimeError){r[[0]][1].run}

    assert_equal(r[[]].length.run, 0)
    assert_equal(r[arr].length.run, arr.length)
    assert_raise(RuntimeError){r[0].length.run}
  end

  def test_stream # from python tests
    arr = (0...10).collect{|x| x}
    assert_equal(r[arr].to_stream.to_array.run, arr)
    assert_equal(r[arr].to_stream.nth(0).run, 0)
    assert_equal(r[arr].to_stream.nth(5).run, 5)
    assert_raise(RuntimeError){r[arr].to_stream.nth([]).run}
    assert_raise(RuntimeError){r[arr].to_stream.nth(0.4).run}
    assert_raise(RuntimeError){r[arr].to_stream.nth(-5).run}
    assert_raise(RuntimeError){r[[0]].to_stream.nth(1).run}
  end

  def test_stream_fancy # from python tests
    def limit(a,c); r[a].to_stream.limit(c).to_array.run; end
    def skip(a,c); r[a].to_stream.skip(c).to_array.run; end

    assert_equal(limit([], 0), [])
    assert_equal(limit([1, 2], 0), [])
    assert_equal(limit([1, 2], 1), [1])
    assert_equal(limit([1, 2], 5), [1, 2])
    assert_raise(RuntimeError){limit([], -1)}

    assert_equal(skip([], 0), [])
    assert_equal(skip([1, 2], 5), [])
    assert_equal(skip([1, 2], 0), [1, 2])
    assert_equal(skip([1, 2], 1), [2])

    def distinct(a); r[a].to_stream.distinct.to_array.run; end
    assert_equal(distinct([]), [])
    assert_equal(distinct([1,2,3]*10), [1,2,3])
    assert_equal(distinct([1, 2, 3, 2]), [1, 2, 3])
    assert_equal(distinct([true, 2, false, 2]), [true, 2, false])
  end

  def test_ordering
    def order(query, *args); query.orderby(*args).run; end
    docs = (0...10).map{|n| {'id' => 100+n, 'a' => n, 'b' => n%3}}
    assert_equal(order(r[docs].to_stream, 'a'), docs.sort_by{|x| x['a']})
    assert_equal(order(r[docs].to_stream,['a', false]), docs.sort_by{|x| x['a']}.reverse)
    assert_equal(order(r[docs].to_stream.filter({'b' => 0}), :a),
                 docs.select{|x| x['b'] == 0}.sort_by{|x| x['a']})
  end

  def test_ops # +,-,%,*,/,<,>,<=,>=,eq,ne,any,all
    assert_equal((r[5] + 3).run, 8)
    assert_equal((r[5].add(3)).run, 8)
    assert_equal(r.add(5,3).run, 8)
    assert_equal(r.add(2,3,3).run, 8)

    assert_equal((r[5] - 3).run, 2)
    assert_equal((r[5].sub(3)).run, 2)
    assert_equal((r[5].subtract(3)).run, 2)
    assert_equal(r.sub(5,3).run, 2)
    assert_equal(r.subtract(5,3).run, 2)

    assert_equal((r[5] % 3).run, 2)
    assert_equal((r[5].mod(3)).run, 2)
    assert_equal((r[5].modulo(3)).run, 2)
    assert_equal(r.mod(5,3).run, 2)
    assert_equal(r.modulo(5,3).run, 2)

    assert_equal((r[5] * 3).run, 15)
    assert_equal((r[5].mul(3)).run, 15)
    assert_equal((r[5].multiply(3)).run, 15)
    assert_equal(r.mul(5,3).run, 15)
    assert_equal(r.multiply(5,3).run, 15)
    assert_equal(r.multiply(5,3,1).run, 15)

    assert_equal((r[15] / 3).run, 5)
    assert_equal((r[15].div(3)).run, 5)
    assert_equal((r[15].divide(3)).run, 5)
    assert_equal(r.div(15,3).run, 5)
    assert_equal(r.divide(15,3).run, 5)

    assert_equal(r.lt(3,2).run,false)
    assert_equal(r.lt(3,3).run,false)
    assert_equal(r.lt(3,4).run,true)
    assert_equal(r[3].lt(2).run,false)
    assert_equal(r[3].lt(3).run,false)
    assert_equal(r[3].lt(4).run,true)
    assert_equal((r[3] < 2).run,false)
    assert_equal((r[3] < 3).run,false)
    assert_equal((r[3] < 4).run,true)

    assert_equal(r.le(3,2).run,false)
    assert_equal(r.le(3,3).run,true)
    assert_equal(r.le(3,4).run,true)
    assert_equal(r[3].le(2).run,false)
    assert_equal(r[3].le(3).run,true)
    assert_equal(r[3].le(4).run,true)
    assert_equal((r[3] <= 2).run,false)
    assert_equal((r[3] <= 3).run,true)
    assert_equal((r[3] <= 4).run,true)

    assert_equal(r.gt(3,2).run,true)
    assert_equal(r.gt(3,3).run,false)
    assert_equal(r.gt(3,4).run,false)
    assert_equal(r[3].gt(2).run,true)
    assert_equal(r[3].gt(3).run,false)
    assert_equal(r[3].gt(4).run,false)
    assert_equal((r[3] > 2).run,true)
    assert_equal((r[3] > 3).run,false)
    assert_equal((r[3] > 4).run,false)

    assert_equal(r.ge(3,2).run,true)
    assert_equal(r.ge(3,3).run,true)
    assert_equal(r.ge(3,4).run,false)
    assert_equal(r[3].ge(2).run,true)
    assert_equal(r[3].ge(3).run,true)
    assert_equal(r[3].ge(4).run,false)
    assert_equal((r[3] >= 2).run,true)
    assert_equal((r[3] >= 3).run,true)
    assert_equal((r[3] >= 4).run,false)

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
    assert_equal((r[true].and(true)).run, true)
    assert_equal((r[true].and(false)).run, false)

    assert_equal(r.any(false, false, false).run, false)
    assert_equal(r.any(false, true, false).run, true)
    assert_equal(r.or(false, false, false).run, false)
    assert_equal(r.or(false, true, false).run, true)
    assert_equal((r[false].or(false)).run, false)
    assert_equal((r[true].or(false)).run, true)
  end

  def test_json # JSON
    assert_equal(r.json('[1,2,3]').run, [1,2,3])
  end

  def test_let # LET, CALL, VAR, NUMBER, STRING
    query = r.let([['a', r.add(1,2)], ['b', r.add(:$a,2)]], r.var('a') + :$b)
    assert_equal(query.run, 8)
  end

  def test_easy_read # TABLE
    assert_equal($data, id_sort(rdb.run))
    assert_equal($data, id_sort(r.db('Welcome-db').table('Welcome-rdb').run))
  end

  def test_error # IF, JSON, ERROR
    query = r.if(r.add(1,2) >= 3, r.json('[1,2,3]'), r.error('unreachable'))
    query_err = r.if(r.add(1,2) > 3, r.json('[1,2,3]'), r.error('reachable'))
    assert_equal(query.run, [1,2,3])
    assert_raise(RuntimeError){assert_equal(query_err.run)}
  end

  def test_array # BOOL, JSON_NULL, ARRAY, ARRAYTOSTREAM
    assert_equal(r.expr([true, false, nil]).run, [true, false, nil])
    assert_equal(r.expr([true, false, nil]).arraytostream.run, [true, false, nil])
    assert_equal(r.expr([true, false, nil]).to_stream.run, [true, false, nil])
  end

  def test_getbykey # OBJECT, GETBYKEY
    query = r.expr({'obj' => rdb.get(0)})
    query2 = r[{'obj' => rdb.get(0)}]
    assert_equal(query.run['obj'], $data[0])
    assert_equal(query2.run['obj'], $data[0])
  end

  def test_map # MAP, FILTER, GETATTR, IMPLICIT_GETATTR, STREAMTOARRAY
    assert_equal(rdb.filter({'num' => 1}).run, [$data[1]])
    assert_equal(id_sort(rdb.filter({'num' => :num}).run), $data)
    query = rdb.orderby(:id).map { |outer_row|
      rdb.filter{r[:id] < outer_row[:id]}.to_array
    }
    assert_equal(query.run[2], $data[0..1])
  end

  def test_reduce # REDUCE, HASATTR, IMPLICIT_HASATTR
    # TODO: Error checking for reduce
    query = rdb.map{r.hasattr(:id)}.reduce(true){|a,b| r.all a,b}
    assert_equal(query.run, true)
    assert_equal(  rdb.map{|row| row['id']}.reduce(0){|a,b| a+b}.run,
                 $data.map{|row| row['id']}.reduce(0){|a,b| a+b})

    assert_equal(rdb.map{r.hasattr(:id)}.run, $data.map{true})
    assert_equal(rdb.map{|row| r.not row.hasattr('id')}.run, $data.map{false})
    assert_equal(rdb.map{|row| r.not row.has('id')}.run, $data.map{false})
    assert_equal(rdb.map{|row| r.not row.attr?('id')}.run, $data.map{false})
    assert_equal(rdb.map{r.has(:id)}.run, $data.map{true})
    assert_equal(rdb.map{r.attr?('id').not}.run, $data.map{false})
  end

  def test_filter # FILTER
    query_5 = rdb.filter{r[:name].eq('5')}
    assert_equal(query_5.run, [$data[5]])
    query_2345 = rdb.filter{|row| r.and r[:id] >= 2,row[:id] <= 5}
    query_234 = query_2345.filter{r[:num].neq(5)}
    query_23 = query_234.filter{|row| r.any row[:num].eq(2),row[:num].equals(3)}
    assert_equal(id_sort(query_2345.run), $data[2..5])
    assert_equal(id_sort(query_234.run), $data[2..4])
    assert_equal(id_sort(query_23.run), $data[2..3])
  end

  def test_slice_streams # SLICE
    arr=[0,1,2,3,4,5]
    assert_equal(r[arr][1].run, 1)
    assert_equal(r[arr].to_stream[1].run, 1)

    assert_equal(r[arr][2...6].run, r[arr][2..-1].run)
    assert_equal(r[arr].to_stream[2...6].run, r[arr].to_stream[2..-1].run)

    assert_equal(r[arr][2...5].run, r[arr][2..4].run)
    assert_equal(r[arr].to_stream[2...5].run, r[arr].to_stream[2..4].run)

    assert_raise(RuntimeError){r[arr][2...0].run}
    assert_raise(RuntimeError){r[arr].to_stream[2...-1].run}
  end

  def test_mapmerge
    assert_equal(r[{:a => 1}].mapmerge({:b => 2}).run, {'a' => 1, 'b' => 2})
    assert_equal(r.mapmerge({:a => 1}, {:a => 2}).run, {'a' => 2})
  end

  def test_orderby # ORDERBY, MAP
    assert_equal(rdb.orderby(:id).run, $data)
    assert_equal(rdb.orderby('id').run, $data)
    assert_equal(rdb.orderby(:id).run, $data)
    assert_equal(rdb.orderby([:id, false]).run, $data.reverse)
    query = rdb.map{r[{:id => r[:id],:num => r[:id].mod(2)}]}.orderby(:num,[:id, false])
    want = $data.map{|o| o['id']}.sort_by{|n| (n%2)*$data.length - n}
    assert_equal(query.run.map{|o| o['id']}, want)
  end

  def test_concatmap # CONCATMAP, DISTINCT
    # TODO_SERV: Using concatmap as a join crashes
    # query = rdb.concatmap{|row| rdb.map{r[:id] * row[:id]}}.distinct
    # nums = $data.map{|o| o['id']}
    # want = nums.map{|n| nums.map{|m| n*m}}.flatten(1).uniq
    # assert_equal(query.run.sort, want.sort)
  end

  def test_range # RANGE
    assert_equal(id_sort(rdb.between(1,3).run), $data[1..3])
    assert_equal(id_sort(rdb.between(2,nil).run), $data[2..-1])
    assert_equal(id_sort(rdb.between(1, 3).run), $data[1..3])
    assert_equal(id_sort(rdb.between(nil, 4).run),$data[0..4])
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
    assert_equal(id_sort(rdb.map{|x| r.js("#{x}")}.run), id_sort(rdb.run))
    assert_equal(id_sort(rdb.map{ r.js("this")}.run), id_sort(rdb.run))
    assert_equal(rdb.map{|x| r.js("#{x}.num")}.run.sort, rdb.map{r[:num]}.run.sort)
    assert_equal(rdb.map{ r.js("this.num")}.run.sort, rdb.map{r[:num]}.run.sort)
    assert_equal(id_sort(rdb.filter{|x| r.js("#{x}.id < 5")}.run),
                 id_sort(rdb.filter{r[:id] < 5}.run))
    assert_equal(id_sort(rdb.filter{ r.js("this.id < 5")}.run),
                 id_sort(rdb.filter{r[:id] < 5}.run))
  end

  def test_pickattrs # PICKATTRS, # UNION, # LENGTH
    q1=r.union(rdb.map{r.pickattrs(:id,:name)}, rdb.map{|row| row.attrs(:id,:num)})
    q2=r.union(rdb.map{r.attrs(:id,:name)}, rdb.map{|row| row.pick(:id,:num)})
    q1v = q1.run.sort_by{|x| x['name'].to_s + ',' + x['id'].to_s}
    assert_equal(q1v, q2.run.sort_by{|x| x['name'].to_s + ',' + x['id'].to_s})
    len = $data.length
    assert_equal(q2.length.run, 2*len)
    assert_equal(q1v.map{|o| o['num']}[len..2*len-1], Array.new(len,nil))
    assert_equal(q1v.map{|o| o['name']}[0..len-1], Array.new(len,nil))
  end

  def test__setup; rdb.delete.run;  rdb.insert($data).run; end

  def test___write_python # from python tests
    rdb.delete.run
    docs = [{"a" => 3, "b" => 10, "id" => 1},
            {"a" => 9, "b" => -5, "id" => 2},
            {"a" => 9, "b" => 3, "id" => 3}]
    assert_equal(rdb.insert(docs).run, {'inserted' => docs.length})
    docs.each {|doc| assert_equal(rdb.get(doc['id']).run, doc)}
    assert_equal(rdb.distinct(:a).run.sort, [3,9].sort)
    assert_equal(rdb.filter({'a' => 3}).run, [docs[0]])
    # TODO_SERV: Issue #922 (exception propagation)
    # assert_raise(RuntimeError){rdb.filter({'a' => rdb.length + ""}).run}
    assert_raise(RuntimeError){rdb.insert({'a' => 3}).run}

    assert_equal(rdb.get(0).run, nil)

    assert_equal(rdb.insert({:id => 100, :text => "グルメ"}).run, {'inserted' => 1})
    assert_equal(rdb.get(100)[:text].run, "グルメ")

    rdb.delete.run
    docs = [{"id" => 1}, {"id" => 2}]
    assert_equal(rdb.insert(docs).run, {'inserted' => docs.length})
    assert_equal(rdb.limit(1).delete.run, {'deleted' => 1})
    assert_equal(rdb.run.length, 1)

    rdb.delete.run
    docs = (0...4).map{|n| {"id" => 100 + n, "a" => n, "b" => n % 3}}
    assert_equal(rdb.insert(docs).run, {'inserted' => docs.length})
    assert_equal(rdb.insertstream(r[docs].to_stream).run, {'inserted' => docs.length})
    docs.each{|doc| assert_equal(rdb.get(doc['id']).run, doc)}

    rdb.delete.run
    docs = (0...10).map{|n| {"id" => 100 + n, "a" => n, "b" => n % 3}}
    assert_equal(rdb.insert(docs).run, {'inserted' => docs.length})
    def filt(expr, fn)
      assert_equal(rdb.filter(exp).orderby(:id).run, docs.select{|x| fn x})
    end

    # TODO: still an error?
    # assert_raise(SyntaxError){r[:a] == 5}
    # assert_raise(SyntaxError){r[:a] != 5}
    assert_equal(id_sort(rdb.filter{r[:a] < 5}.run), docs.select{|x| x['a'] < 5})
    assert_equal(id_sort(rdb.filter{r[:a] <= 5}.run), docs.select{|x| x['a'] <= 5})
    assert_equal(id_sort(rdb.filter{r[:a] > 5}.run), docs.select{|x| x['a'] > 5})
    assert_equal(id_sort(rdb.filter{r[:a] >= 5}.run), docs.select{|x| x['a'] >= 5})
    assert_raise(ArgumentError){5 < r[:a]}
    assert_raise(ArgumentError){5 <= r[:a]}
    assert_raise(ArgumentError){5 > r[:a]}
    assert_raise(ArgumentError){5 >= r[:a]}
    assert_equal(id_sort(rdb.filter{r[:a].eq(r[:b])}.run),
                 docs.select{|x| x['a'] == x['b']})

    assert_equal(-r[5].run, r[-5].run)
    assert_equal(+r[5].run, r[+5].run)

    assert_equal((r[3]+4).run, 7)
    assert_equal((r[3]-4).run, -1)
    assert_equal((r[3]*4).run, 12)
    assert_equal((r[3]/4).run, 3.0/4)
    assert_equal((r[3]%2).run, 3%2)
    assert_raise(TypeError){4+r[3]}
    assert_raise(TypeError){4-r[3]}
    assert_raise(TypeError){4*r[3]}
    assert_raise(TypeError){4/r[3]}
    assert_raise(TypeError){4%r[3]}

    assert_equal(((r[3]+4)*-r[6]*(r[-5]+3)).run, 84)

    rdb.delete.run
    docs = (0...10).map{|n| {"id" => 100 + n, "a" => n, "b" => n % 3}}
    assert_equal(rdb.insert(docs).run, {'inserted' => docs.length})
    assert_equal(rdb.mutate{|x| x}.run,
                 {'modified' => docs.length, 'deleted' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb.run), docs)
    assert_equal(rdb.update{nil}.run, {'updated' => 0, 'skipped' => 10, 'errors' => 0})

    res = rdb.update{|row| r.if((row[:id]%2).eq(0), {:id => -1}, row)}.run;
    assert_not_nil(res['first_error'])
    filtered_res = res.reject {|k,v| k == 'first_error'}
    assert_not_equal(filtered_res, res)
    assert_equal(filtered_res, {'updated' => 5, 'skipped' => 0, 'errors' => 5})

    assert_equal(id_sort(rdb.run), docs);
  end

  def test_getitem #from python tests
    arr = (0...10).map{|x| x}
    [0..-1, 2..-1, 0...2, -1..-1, 0...-1, 3...5, 3, -1].each {|rng|
      assert_equal(r[arr][rng].run, arr[rng])}
    obj = {:a => 3, :b => 4}
    [:a, :b].each {|attr|
      assert_equal(r[obj][attr].run, obj[attr])}
    assert_raise(RuntimeError){r[obj][:c].run}
  end

  def test_stream_getitem #from python tests
    arr = (0...10).map{|x| x}
    [0..-1, 3..-1, 0...3, 4...6, 3].each {|rng|
      assert_equal(r[arr].to_stream[rng].run, arr[rng])}
    assert_raise(ArgumentError){r[arr].to_stream[4...'a'].run}
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

    assert_equal(r.db(db_name).table(table_name).insert({:id => 0}).run,
                 {'inserted' => 1})
    assert_equal(r.db(db_name).table(table_name).run, [{'id' => 0}])
    assert_equal(r.db(db_name).create_table(table_name+table_name).run, nil)
    assert_equal(r.db(db_name).list_tables.run.length, 2)

    assert_raise(RuntimeError){r.db('').table('').run}
    assert_raise(RuntimeError){r.db('Welcome-db').table(table_name).run}
    assert_raise(RuntimeError){r.db(db_name).table('').run}

    assert_equal(r.db(db_name).drop_table(table_name+table_name).run, nil)
    assert_raise(RuntimeError){r.db(db_name).table(table_name+table_name).run}
    assert_equal(r.drop_db(db_name).run, nil)
    assert_equal(r.list_dbs.run.sort, orig_dbs.sort)
    assert_raise(RuntimeError){r.db(db_name).table(table_name).run}

    assert_equal(r.create_db(db_name).run, nil)
    assert_raise(RuntimeError){r.db(db_name).table(table_name).run}
    assert_equal(r.db(db_name).create_table(table_name).run, nil)
    assert_equal(r.db(db_name).table(table_name).run, [])
    rdb2 = r.db(db_name).table(table_name)

    $data = []
    len = 10
    Array.new(len).each_index{|i| $data << {'id'=>i,'num'=>i,'name'=>i.to_s}}

    # INSERT, UPDATE
    assert_equal(rdb2.insert($data).run['inserted'], len)
    assert_equal(rdb2.insert($data + $data).run['inserted'], len*2)
    assert_equal(id_sort(rdb2.run), $data)
    assert_equal(rdb2.insert({:id => 0, :broken => true}).run['inserted'], 1)
    assert_equal(rdb2.insert({:id => 1, :broken => true}).run['inserted'], 1)
    assert_equal(id_sort(rdb2.run)[2..len-1], $data[2..len-1])
    assert_equal(id_sort(rdb2.run)[0]['broken'], true)
    assert_equal(id_sort(rdb2.run)[1]['broken'], true)

    mutate = rdb2.filter{r[:id].eq(0)}.mutate{$data[0]}.run
    assert_equal(mutate, {"modified"=>1, "deleted"=>0, 'errors' => 0})
    update = rdb2.update{|x| r.if(x.attr?(:broken), $data[1], x)}.run
    assert_equal(update, {'errors' => 0, 'updated' => len, 'skipped' => 0})
    mutate = rdb2.mutate{|x| {:id => x[:id], :num => x[:num], :name => x[:name]}}.run
    assert_equal(mutate, {"modified"=>10, "deleted"=>0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run), $data)

    mutate = rdb2.mutate{|row| r.if(row[:id].eq(4) | row[:id].eq(5),
                                    {:id => -1}, row)}.run
    assert_not_nil(mutate['first_error'])
    filtered_mutate = mutate.reject {|k,v| k == 'first_error'}
    assert_equal(filtered_mutate, {'modified' => 8, 'deleted' => 0, 'errors' => 2})
    assert_equal(rdb2.get(5).update{{:num => "5"}}.run,
                 {'updated' => 1, 'skipped' => 0, 'errors' => 0})
    mutate = rdb2.mutate{|row| r.mapmerge(row, {:num => row[:num]*2})}.run
    assert_not_nil(mutate['first_error'])
    filtered_mutate = mutate.reject {|k,v| k == 'first_error'}
    assert_equal(filtered_mutate, {'modified' => 9, 'deleted' => 0, 'errors' => 1})
    assert_equal(rdb2.get(5).run['num'], '5')
    assert_equal(rdb2.mutate{|row|
                   r.if(row[:id].eq(5),
                        $data[5],
                        row.mapmerge({:num => row[:num]/2}))}.run,
                 {'modified' => 10, 'deleted' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run), $data);

    update = rdb2.get(0).update{{:num => 2}}.run
    assert_equal(update, {'errors' => 0, 'updated' => 1, 'skipped' => 0})
    assert_equal(rdb2.get(0).run['num'], 2);
    update = rdb2.get(0).update{nil}.run
    assert_equal(update, {'errors' => 0, 'updated' => 0, 'skipped' => 1})
    assert_equal(rdb2.get(0).mutate{$data[0]}.run,
                 {"errors"=>0, "deleted"=>0, "modified"=>1})
    assert_equal(id_sort(rdb2.run), $data)

    # DELETE
    assert_equal(rdb2.filter{r[:id] < 5}.delete.run['deleted'], 5)
    assert_equal(id_sort(rdb2.run), $data[5..-1])
    assert_equal(rdb2.delete.run['deleted'], len-5)
    assert_equal(rdb2.run, [])

    # INSERTSTREAM
    assert_equal(rdb2.insertstream(r[$data].to_stream).run['inserted'], len)
    rdb2.insert($data).run
    assert_equal(id_sort(rdb2.run), $data)

    # MUTATE
    assert_equal(rdb2.mutate{|row| row}.run,
                 {'modified' => len, 'deleted' => 0, 'errors' => 0})
    assert_equal(id_sort(rdb2.run), $data)
    assert_equal(rdb2.mutate{|row| r.if(row[:id] < 5, nil, row)}.run,
                 {'modified' => 5, 'deleted' => len-5, 'errors' => 0})
    assert_equal(id_sort(rdb2.run), $data[5..-1])
    assert_equal(rdb2.insert($data[0...5]).run, {'inserted' => 5})

    # FOREACH, POINTDELETE
    # TODO_SERV: Return value of foreach should change (Issue #874)
    rdb2.foreach{|row| [rdb2.get(row[:id]).delete, rdb2.insert(row)]}.run
    assert_equal(id_sort(rdb2.run), $data)
    rdb2.foreach{|row| rdb2.get(row[:id]).delete}.run
    assert_equal(id_sort(rdb2.run), [])

    rdb2.insert($data).run
    assert_equal(id_sort(rdb2.run), $data)
    query = rdb2.get(0).update{{:id => 0, :broken => 5}}
    assert_equal(query.run, {'updated'=>1,'errors'=>0,'skipped'=>0})
    query = rdb2.insert($data[0])
    assert_equal(query.run, {'inserted'=>1})
    assert_equal(id_sort(rdb2.run), $data)

    assert_equal(rdb2.get(0).mutate{nil}.run,
                 {'modified' => 0, 'deleted' => 1, 'errors' => 0})
    assert_equal(id_sort(rdb2.run), $data[1..-1])
    assert_raise(RuntimeError){rdb2.get(1).mutate{{:id => -1}}.run}
    assert_raise(RuntimeError){rdb2.get(1).mutate{|row| {:name => row[:name]*2}}.run}
    assert_equal(rdb2.get(1).mutate{|row| row.mapmerge({:num => 2})}.run,
                 {'modified' => 1, 'deleted' => 0, 'errors' => 0})
    assert_equal(rdb2.get(1).run['num'], 2);
    assert_equal(rdb2.insert($data[0...2]).run, {'inserted' => 2})

    assert_equal(id_sort(rdb2.run), $data)
  end
end
