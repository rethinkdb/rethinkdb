load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/rethinkdb.rb'
r = RethinkDB::RQL
################################################################################
#                                  TERM TESTS                                  #
################################################################################
def p? x; x; end
r.javascript('javascript').query ;p? :JAVASCRIPT

################################################################################
#                                 BUILTIN TESTS                                #
################################################################################
r.var('v').pickattrs('a').query ;p? :PICKATTRS
r.var('v').attrs('a').query ;p? :PICKATTRS
r.attrs('a').query ;p? :IMPLICIT_PICKATTRS

# These are going away?
#MAPMERGE
#ARRAYAPPEND
#ARRAYCONCAT
#ARRAYSLICE
#ARRAYNTH
#ARRAYLENGTH

r.limit(:$var).query ;p? :LIMIT
r.length(:$var).query ;p? :LENGTH
r.union(:$var1, :$var2).query ;p? :LENGTH
r.nth(:$var1).query ;p? :LENGTH
r.streamtoarray(r.table('','Welcome').all).query ;p? :STREAMTOARRAY

#GROUPEDMAPREDUCE -- how does this work?

r.or(true, true, false).query ;p? :ANY
r.expr(true).or(false).query ;p? :ANY
r.all(true, true, false).query ;p? :ALL
r.expr(true).and(false).query ;p? :ALL

r.table('','Welcome').range('id', 1, 2).query ;p? :RANGE
r.table('','Welcome').range(:id, 1, 2).query ;p? :RANGE
r.table('','Welcome').range({:attrname => :id, :upperbound => 2}).query ;p? :RANGE

#SKIP -- how does this work?

################################################################################
#                                 WRITE QUERY                                  #
################################################################################

r.db('').Welcome.update{|x| x}.query ;p? :UPDATE
r.db('').Welcome.filter{|x| x[:id] < 5}.update{|x| x}.query ;p? :UPDATE

r.db('').Welcome.delete.query ;p? :DELETE
r.db('').Welcome.filter{|x| x[:id] < 5}.delete.query ;p? :DELETE

r.db('').Welcome.mutate{|x| nil}.query ;p? :MUTATE
r.db('').Welcome.filter{|x| x[:id] < 5}.mutate{|x| nil}.query ;p? :MUTATE

r.db('').Welcome.insert([{:id => 1, :name => 'tst'}]).query ;p? :INSERT
r.db('').Welcome.insert({:id => 1, :name => 'tst'}).query ;p? :INSERT

r.db('').Welcome.insertstream(r.db('').other_tbl).query ;p? :INSERTSTREAM

r.db('').Welcome.foreach {|row|
  [r.db('').other_tbl1.insert(row),
   r.db('').other_tbl2.insert(row)]
}.query ;p? :FOREACH
r.db('').Welcome.foreach {|x| [r.db('').other_tbl.insert [x]]}.query ;p? :FOREACH
r.db('').Welcome.foreach {|row| [r.db('').other_tbl.insert row]}.query ;p? :FOREACH
r.db('').Welcome.foreach {|row| r.db('').other_tbl.insert row}.query ;p? :FOREACH

r.db('').Welcome.pointupdate(:address, 'Bob'){'addr2'}.query ;p? :POINTUPDATE
r.db('').Welcome.pointupdate(:address, 'Bob'){|addr| addr}.query ;p? :POINTUPDATE

r.db('').Welcome.pointdelete(:address, 'Bob').query ;p? :POINTDELETE

r.db('').Welcome.pointmutate(:address, 'Bob'){'addr2'}.query ;p? :POINTMUTATE
r.db('').Welcome.pointmutate(:address, 'Bob'){nil}.query ;p? :POINTMUTATE
r.db('').Welcome.pointmutate(:address, 'Bob'){|addr| addr}.query ;p? :POINTMUTATE
r.db('').Welcome.pointmutate(:address, 'Bob'){|addr| nil}.query ;p? :POINTMUTATE

################################################################################
#                                 CONNECTION                                   #
################################################################################

require 'test/unit'
class ClientTest < Test::Unit::TestCase
  def r; RethinkDB::RQL; end
  def rdb; r.db('').Welcome; end
  @@c = RethinkDB::Connection.new('localhost', 64346)
  def c; @@c; end

  def test_let #LET, CALL, ADD, VAR, NUMBER, STRING
    query = r.let([['a', r.add(1,2)], ['b', r.add(:$a,2)]], r.var('a') + :$b)
    assert_equal(query.run, 8)
  end

  def test_easy_read #TABLE
    assert_equal($welcome_data, rdb.run)
    assert_equal($welcome_data, r.table('', 'Welcome').run)
    assert_equal($welcome_data, r.table({:table_name => 'Welcome'}).run)
  end

  def test_error #IF, JSON, ERROR
    query = r.if(r.add(1,2) >= 3, r.json('[1,2,3]'), r.error('unreachable'))
    query_err = r.if(r.add(1,2) > 3, r.json('[1,2,3]'), r.error('unreachable'))
    assert_equal(query.run, [1,2,3])
    assert_raise(RuntimeError){assert_equal(query_err.run)}
  end

  def test_array #BOOL, JSON_NULL, ARRAY, ARRAYTOSTREAM
    assert_equal(r.expr([true, false, nil]).run, [true, false, nil])
    assert_equal(r.arraytostream(r.expr([true, false, nil])).run, [true, false, nil])
  end

  def test_getbykey #OBJECT, GETBYKEY
    query = r.expr({'obj' => rdb.getbykey(:id, 0)})
    query2 = r[{'obj' => rdb.getbykey(:id, 0)}]
    assert_equal(query.run['obj'], $welcome_data[0])
    assert_equal(query2.run['obj'], $welcome_data[0])
  end

  def test_map #MAP, FILTER, LT, GETATTR, IMPLICIT_GETATTR
    query = rdb.map { |outer_row|
      r.streamtoarray(rdb.filter{r[:id] < outer_row[:id]})
    }
    query2 = rdb.map { |outer_row|
      r.streamtoarray(rdb.filter{r.lt(r[:id],outer_row[:id])})
    }
    query3 = rdb.map { |outer_row|
      r.streamtoarray(rdb.filter{r.attr('id').lt(outer_row.getattr(:id))})
    }
    assert_equal(query.run[2], $welcome_data[0..1])
    assert_equal(query2.run[2], $welcome_data[0..1])
    assert_equal(query3.run[2], $welcome_data[0..1])
  end

  def test_reduce #REDUCE, NOT, HASATTR, IMPLICIT_HASATTR
    #REDUCE NOT IMPLEMENTED
    #query = rdb.map{r.hasattr(:id)}.reduce(true){|a,b| r.all a,b}
    assert_equal(rdb.map{r.hasattr(:id)}.run, $welcome_data.map{true})
    assert_equal(rdb.map{|row| r.not row.hasattr('id')}.run, $welcome_data.map{false})
    assert_equal(rdb.map{r.attr?(:id)}.run, $welcome_data.map{true})
    assert_equal(rdb.map{r.attr?('id').not}.run, $welcome_data.map{false})
  end

  def test_filter #FILTER, ANY, ALL, EQ
    #BROKEN: Can't filter on strings
    #query_5 = rdb.filter{r[:name].eq('5')}
    query_2345 = rdb.filter{|row| r.and r[:id] >= 2,row[:id] <= 5}
    query_234 = query_2345.filter{r[:num].neq(5)}
    query_23 = query_234.filter{|row| r.any row[:num].eq(2),row[:num].equals(3)}
    query_23_alt =
      query_234.filter{|row| r.any((r.eq(row[:num],2)),(r.equals(row[:num],3)))}
    assert_equal(query_2345.run, $welcome_data[2..5])
    assert_equal(query_234.run, $welcome_data[2..4])
    assert_equal(query_23.run, $welcome_data[2..3])
    assert_equal(query_23_alt.run, $welcome_data[2..3])
  end

  def test_orderby #ORDERBY, MAP
    assert_equal(rdb.orderby(:id).run, $welcome_data)
    assert_equal(rdb.orderby('id').run, $welcome_data)
    assert_equal(rdb.orderby([:id]).run, $welcome_data)
    assert_equal(rdb.orderby([:id,true]).run, $welcome_data)
    assert_equal(rdb.orderby([:id,false]).run, $welcome_data.reverse)
    query = rdb.map{r[{:id => r[:id],:num => r[:id].mod(2)}]}.orderby(:num,['id',false])
    want = $welcome_data.map{|o| o['id']}.sort_by{|n| (n%2)*$welcome_data.length - n}
    assert_equal(query.run.map{|o| o['id']}, want)
  end

  def test_concatmap #CONCATMAP, DISTINCT, MULTIPLY
    query = rdb.concatmap{|row| rdb.map{r[:id] * row[:id]}}.distinct
    nums = $welcome_data.map{|o| o['id']}
    want = nums.map{|n| nums.map{|m| n*m}}.flatten(1).uniq
    assert_equal(query.run.sort, want.sort)
  end

  def test_ops #+,-,%,*,/,<,>,<=,>=
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
end
end


c = RethinkDB::Connection.new('localhost', 64346)
rdb = r.db('').Welcome
$welcome_data = []
Array.new(10).each_index{|i| $welcome_data << {'id' => i, 'num' => i, 'name' => i.to_s}}
c.run(rdb.insert($welcome_data).query)
