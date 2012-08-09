#load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/rethinkdb.rb'
load 'rethinkdb_shortcuts.rb'
r = RethinkDB::RQL
# BIG TODO:
#   * Make Connection work with clusters, minimize network hops,
#     etc. etc. like python client does.
#   * CREATE, DROP, LIST, etc. for table/namespace creation

# TODO:
# Confirm these are going away:
#   * MAPMERGE
#   * ARRAYAPPEND
#   * ARRAYCONCAT
#   * ARRAYSLICE
#   * ARRAYNTH
#   * ARRAYLENGTH
# Figure out how to use:
#   * JAVASCRIPT
#   * GROUPEDMAPREDUCE
# Add tests once completed/fixed:
#   * REDUCE
#   * MUTATE
#   * FOREACH
#   * POINTUPDATE
#   * POINTMUTATE
#   * SOME MERGE

################################################################################
#                                 CONNECTION                                   #
################################################################################

require 'test/unit'
class ClientTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts_Mixin
  def rdb; r.db('').Welcome; end
  @@c = RethinkDB::Connection.new('localhost', 64346)
  def c; @@c; end

  def test_ops #+,-,%,*,/,<,>,<=,>=,eq,ne,any,all
    assert_equal((r[5] + 3).run, 8)
    assert_equal((r[5].add(3)).run, 8)
    assert_equal(r.add(5,3).run, 8)

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

  def test_let #LET, CALL, VAR, NUMBER, STRING
    query = r.let([['a', r.add(1,2)], ['b', r.add(:$a,2)]], r.var('a') + :$b)
    assert_equal(query.run, 8)
  end

  def test_easy_read #TABLE
    assert_equal($data, rdb.run)
    assert_equal($data, r.table('', 'Welcome').run)
    assert_equal($data, r.table({:table_name => 'Welcome'}).run)
  end

  def test_error #IF, JSON, ERROR
    query = r.if(r.add(1,2) >= 3, r.json('[1,2,3]'), r.error('unreachable'))
    query_err = r.if(r.add(1,2) > 3, r.json('[1,2,3]'), r.error('reachable'))
    assert_equal(query.run, [1,2,3])
    assert_raise(RuntimeError){assert_equal(query_err.run)}
  end

  def test_array #BOOL, JSON_NULL, ARRAY, ARRAYTOSTREAM
    assert_equal(r.expr([true, false, nil]).run, [true, false, nil])
    assert_equal(r.arraytostream(r.expr([true, false, nil])).run, [true, false, nil])
  end

  def test_getbykey #OBJECT, GETBYKEY
    query = r.expr({'obj' => rdb.get(0)})
    query2 = r[{'obj' => rdb.get(0)}]
    assert_equal(query.run['obj'], $data[0])
    assert_equal(query2.run['obj'], $data[0])
  end

  def test_map #MAP, FILTER, GETATTR, IMPLICIT_GETATTR
    assert_equal(rdb.filter({'num' => 1}).run, [$data[1]])
    assert_equal(rdb.filter({'num' => r[:num]}).run, $data)
    query = rdb.map { |outer_row|
      r.streamtoarray(rdb.filter{r[:id] < outer_row[:id]})
    }
    query2 = rdb.map { |outer_row|
      r.streamtoarray(rdb.filter{r.lt(r[:id],outer_row[:id])})
    }
    query3 = rdb.map { |outer_row|
      r.streamtoarray(rdb.filter{r.attr('id').lt(outer_row.getattr(:id))})
    }
    assert_equal(query.run[2], $data[0..1])
    assert_equal(query2.run[2], $data[0..1])
    assert_equal(query3.run[2], $data[0..1])
  end

  def test_reduce #REDUCE, HASATTR, IMPLICIT_HASATTR
    #REDUCE NOT IMPLEMENTED
    #query = rdb.map{r.hasattr(:id)}.reduce(true){|a,b| r.all a,b}
    assert_equal(rdb.map{r.hasattr(:id)}.run, $data.map{true})
    assert_equal(rdb.map{|row| r.not row.hasattr('id')}.run, $data.map{false})
    assert_equal(rdb.map{r.attr?(:id)}.run, $data.map{true})
    assert_equal(rdb.map{r.attr?('id').not}.run, $data.map{false})
  end

  def test_filter #FILTER
    #BROKEN: Can't filter on strings
    #query_5 = rdb.filter{r[:name].eq('5')}
    query_2345 = rdb.filter{|row| r.and r[:id] >= 2,row[:id] <= 5}
    query_234 = query_2345.filter{r[:num].neq(5)}
    query_23 = query_234.filter{|row| r.any row[:num].eq(2),row[:num].equals(3)}
    assert_equal(query_2345.run, $data[2..5])
    assert_equal(query_234.run, $data[2..4])
    assert_equal(query_23.run, $data[2..3])
  end

  def test_orderby #ORDERBY, MAP
    assert_equal(rdb.orderby(:id).run, $data)
    assert_equal(rdb.orderby('id').run, $data)
    assert_equal(rdb.orderby([:id]).run, $data)
    assert_equal(rdb.orderby([:id,true]).run, $data)
    assert_equal(rdb.orderby([:id,false]).run, $data.reverse)
    query = rdb.map{r[{:id => r[:id],:num => r[:id].mod(2)}]}.orderby(:num,['id',false])
    want = $data.map{|o| o['id']}.sort_by{|n| (n%2)*$data.length - n}
    assert_equal(query.run.map{|o| o['id']}, want)
  end

  def test_concatmap #CONCATMAP, DISTINCT
    query = rdb.concatmap{|row| rdb.map{r[:id] * row[:id]}}.distinct
    nums = $data.map{|o| o['id']}
    want = nums.map{|n| nums.map{|m| n*m}}.flatten(1).uniq
    assert_equal(query.run.sort, want.sort)
  end

  def test_range #RANGE
    assert_equal(rdb.between(1,3).run, $data[1..3])
    assert_equal(rdb.between(2,nil).run, $data[2..-1])
    assert_equal(rdb.range(:id, 1, 3).run, $data[1..3])
    assert_equal(rdb.range({:attrname => :id, :upperbound => 4}).run,$data[0..4])
  end

  def test_limit #LIMIT, SKIP
    assert_equal(rdb.limit(5).skip(2).run, $data[2..4])
  end

  def test_nth #NTH
    assert_equal(rdb.nth(2).run, $data[2])
  end

  def test_pickattrs #PICKATTRS, #UNION, #LENGTH
    q1=r.union(rdb.map{r.pickattrs(:id,:name)}, rdb.map{|row| row.pickattrs(:id,:num)})
    q2=r.union(rdb.map{r.attrs(:id,:name)}, rdb.map{|row| row.attrs(:id,:num)})
    q1v = q1.run
    assert_equal(q1v, q2.run)
    len = $data.length
    assert_equal(q2.length.run, 2*len)
    assert_equal(q1v.map{|o| o['num']}[0..len-1], Array.new(len,nil))
    assert_equal(q1v.map{|o| o['name']}[len..2*len-1], Array.new(len,nil))
  end

  def test__setup; rdb.insert($data).run; end

  def test___write #three underscores so it runs first
    rdb.delete.run
    $data = []
    len = 10
    Array.new(len).each_index{|i| $data << {'id'=>i,'num'=>i,'name'=>i.to_s}}

    #INSERT, UPDATE
    assert_equal(rdb.insert($data).run['inserted'], $data.length)
    assert_equal(rdb.run, $data)
    assert_equal(rdb.insert({:id => 0, :broken => true}).run['inserted'], 1)
    assert_equal(rdb.insert({:id => 1, :broken => true}).run['inserted'], 1)
    assert_equal(rdb.run[2..len-1], $data[2..len-1])
    assert_equal(rdb.run[0]['broken'], true)
    assert_equal(rdb.run[1]['broken'], true)
    #PP.pp rdb.update{|x| r.if(x.attr?(:broken), $data[0], nil)}.run
    update = rdb.filter{r[:id].eq(0)}.update{$data[0]}.run
    assert_equal(update, {'errors' => 0, 'updated' => 1})
    update = rdb.update{|x| r.if(x.attr?(:broken), $data[1], x)}.run
    assert_equal(update, {'errors' => 0, 'updated' => len})
    assert_equal(rdb.run, $data)

    #DELETE
    assert_equal(rdb.filter{r[:id] < 5}.delete.run['deleted'], 5)
    assert_equal(rdb.run, $data[5..-1])
    assert_equal(rdb.delete.run['deleted'], len-5)
    assert_equal(rdb.run, [])

    #INSERTSTREAM
    assert_equal(rdb.insertstream(r.arraytostream r[$data]).run['inserted'], len)
    assert_equal(rdb.run, $data)

    #MUTATE -- need fix

    #FOREACH, POINTDELETE
    #PP.pp rdb.foreach{rdb.pointdelete(:id, r[:id])}.run
    #PP.pp rdb.foreach{|x| rdb.foreach{|y| rdb.insert({:id => x[:id]*y[:id]})}}.run
    # RETURN VALUE SHOULD CHANGE
    rdb.foreach{|row| [rdb.pointdelete(:id, row[:id]), rdb.insert(row)]}.run
    assert_equal(rdb.run, $data)
    rdb.foreach{|row| rdb.pointdelete(:id, row[:id])}.run
    assert_equal(rdb.run, [])

    rdb.insert($data).run
    assert_equal(rdb.run, $data)
    query = rdb.pointupdate(:id, 0){r[{:id => 0, :broken => 5}]}
    assert_equal(query.run, {'updated'=>1,'errors'=>0})
    query = rdb.pointupdate(:id, 0){|row|
      r.if(row.attr?(:broken), $data[0], r.error('unreachable'))}
    assert_equal(query.run, {'updated'=>1,'errors'=>0})
    assert_equal(rdb.run, $data)

    #POINTMUTATE -- unimplemented

    assert_equal(rdb.run, $data)
  end
end
