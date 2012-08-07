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
r.not(true).query ;p? :NOT
r.expr(true).not.query ;p? :NOT

r.var('v').hasattr('a').query ;p? :HASATTR
r.var('v').attr?('a').query ;p? :HASATTR
r.attr?('a').query ;p? :IMPLICIT_HASATTR

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

r.subtract(1, 2).query ;p? :SUBTRACT
r.expr(1).subtract(2).query ;p? :SUBTRACT
(r.expr(1) - 2).query ;p? :SUBTRACT
r.multiply(1, 2).query ;p? :MULTIPLY
r.expr(1).multiply(2).query ;p? :MULTIPLY
(r.expr(1) * 2).query ;p? :MULTIPLY
r.divide(1, 2).query ;p? :DIVIDE
r.expr(1).divide(2).query ;p? :DIVIDE
(r.expr(1) / 2).query ;p? :DIVIDE
r.modulo(1, 2).query ;p? :MODULO
r.expr(1).modulo(2).query ;p? :MODULO
(r.expr(1) % 2).query ;p? :MODULO

r.eq(1, 2).query ;p? :COMPARE_EQ
r.expr(1).eq(2).query ;p? :COMPARE_EQ
r.equals(1, 2).query ;p? :COMPARE_EQ
r.expr(1).equals(2).query ;p? :COMPARE_EQ

r.ne(1, 2).query ;p? :COMPARE_NE
r.expr(1).ne(2).query ;p? :COMPARE_NE
r.neq(1, 2).query ;p? :COMPARE_NE
r.expr(1).neq(2).query ;p? :COMPARE_NE

r.lt(1, 2).query ;p? :COMPARE_LT
r.expr(1).lt(2).query ;p? :COMPARE_LT
(r.expr(1) < 2).query ;p? :COMPARE_LT

r.le(1, 2).query ;p? :COMPARE_LE
r.expr(1).le(2).query ;p? :COMPARE_LE
(r.expr(1) <= 2).query ;p? :COMPARE_LE

r.gt(1, 2).query ;p? :COMPARE_GT
r.expr(1).gt(2).query ;p? :COMPARE_GT
(r.expr(1) > 2).query ;p? :COMPARE_GT

r.ge(1, 2).query ;p? :COMPARE_GE
r.expr(1).ge(2).query ;p? :COMPARE_GE
(r.expr(1) >= 2).query ;p? :COMPARE_GE

r.table('','Welcome').filter { |row| row[:id].eq(1) }.query ;p? :FILTER
r.table('','Welcome').map { |row| row}.query ;p? :MAP
r.table('','Welcome').concatmap { |row|
  r.table('','Other').filter { |row2|
    row[:id] > row2[:id]
  }
}.query ;p? :CONCATMAP

r.table('','Welcome').orderby([:name], :age).query ;p? :ORDERBY
r.table('','Welcome').orderby('name', 'age').query ;p? :ORDERBY
r.table('','Welcome').orderby([:name, false], :age).query ;p? :ORDERBY
r.table('','Welcome').orderby('name', ['age', false]).query ;p? :ORDERBY

r.distinct('a', 'b').query ;p? :DISTINCT
r.limit(:$var).query ;p? :LIMIT
r.length(:$var).query ;p? :LENGTH
r.union(:$var1, :$var2).query ;p? :LENGTH
r.nth(:$var1).query ;p? :LENGTH
r.streamtoarray(r.table('','Welcome').all).query ;p? :STREAMTOARRAY

r.table('','Welcome').reduce(1,'a','b'){r.var('a')+r.var('b')}.query ;p? :REDUCE
r.table('','Welcome').reduce(0){|a,b| a+b[:size]}.query ;p? :REDUCE

#GROUPEDMAPREDUCE -- how does this work?

r.any(true, true, false).query ;p? :ANY
r.or(true, true, false).query ;p? :ANY
r.expr(true).or(false).query ;p? :ANY
r.all(true, true, false).query ;p? :ALL
r.and(true, true, false).query ;p? :ALL
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
  @@c = RethinkDB::Connection.new('localhost', 64346)
  def c; @@c; end

  def test_let #LET, CALL, ADD, VAR, NUMBER, STRING
    query = r.let([['a', r.add(1,2)], ['b', r.add(:$a,2)]], r.var('a') + :$b)
    assert_equal(query.run, 8)
  end

  def test_easy_read #TABLE
    assert_equal($welcome_data, r.db('').Welcome.run)
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
    query = r.expr({'obj' => r.db('').Welcome.getbykey(:id, 0)})
    query2 = r[{'obj' => r.db('').Welcome.getbykey(:id, 0)}]
    assert_equal(query.run['obj'], $welcome_data[0])
    assert_equal(query2.run['obj'], $welcome_data[0])
  end

  def test_map #MAP, FILTER, LT, GETATTR, IMPLICIT_GETATTR
    query = r.db('').Welcome.map { |outer_row|
      r.streamtoarray(r.db('').Welcome.filter{r[:id] < outer_row[:id]})
    }
    query2 = r.db('').Welcome.map { |outer_row|
      r.streamtoarray(r.db('').Welcome.filter{r.attr('id') < outer_row.getattr(:id)})
    }
    assert_equal(query.run[2], $welcome_data[0..1])
    assert_equal(query2.run[2], $welcome_data[0..1])
  end
end


c = RethinkDB::Connection.new('localhost', 64346)
$welcome_data = []
Array.new(10).each_index{|i| $welcome_data << {'id' => i, 'name' => i.to_s}}
c.run(r.db('').Welcome.insert($welcome_data).query)
c.run(r.db('').Welcome)
r.db('').Welcome.run
query = r.db('').Welcome.map { |row|
  r.streamtoarray(r.db('').Welcome.filter{row[:id] < r[:id]})
}
query.run
# # c.run(r.db('').Welcome).length
# # c.run(r.add(1,2))
# # c.run(r.let([['a', r.add(1,2)],
# #              ['b', r.add(:$a, 3)]],
# #             r.add(:$a, :$b)))
# c.run(r.add(1,2))


