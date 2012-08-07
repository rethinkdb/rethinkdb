load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/rethinkdb.rb'
r = RethinkDB::RQL
################################################################################
#                                  TERM TESTS                                  #
################################################################################
def p? x; p x; end
r.var('x').query ;p? :VAR
r.expr(:$x).query ;p? :VAR
r.let([["a", 1],
       ["b", 2]],
      r.all(r.var('a').eq(1), r.var('b').eq(2))).query ;p? :LET
#CALL not done explicitly
r.if(r.var('a'), 1, '2').query ;p? :IF
r.error('e').query ;p? :ERROR
r.expr(5).query ;p? :NUMBER
r.expr('s').query ;p? :STRING
r.json('{type : "json"}').query ;p? :JSON
r.expr(true).query ;p? :BOOL
r.expr(false).query ;p? :BOOL
r.expr(nil).query ;p? :JSON_NULL
r.expr([1, 2, [4, 5]]).query ;p? :ARRAY
r.expr({'a' => 1, 'b' => '2', 'c' => { 'nested' => true }}).query ;p? :OBJECT
r.table('a', 'b').getbykey('attr', 5).query ;p? :GETBYKEY
r.table('a', 'b').query ;p? :TABLE
r.table({:table_name => 'b'}).query ;p? :TABLE
r.javascript('javascript').query ;p? :JAVASCRIPT

################################################################################
#                                 BUILTIN TESTS                                #
################################################################################
r.not(true).query ;p? :NOT
r.expr(true).not.query ;p? :NOT

r.var('v')['a'].query ;p? :GETATTR
r.var('v')[:a].query ;p? :GETATTR
r.var('v').attr('a').query ;p? :GETATTR
r.var('v').getattr('a').query ;p? :GETATTR
r.attr('a').query ;p? :IMPLICIT_GETATTR
r.expr(:a).query ;p? :IMPLICIT_GETATTR

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

r.add(1, 2).query ;p? :ADD
r.expr(1).add(2).query ;p? :ADD
(r.expr(1) + 2).query ;p? :ADD
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
r.arraytostream(r.expr [1,2,3]).query ;p? :ARRAYTOSTREAM

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
$r = RethinkDB::RQL
class ClientTest < Test::Unit::TestCase
  @@c = RethinkDB::Connection.new('localhost', 64346)
  def test_let
    assert_equal(@@c.run($r.add(1,2)), 3)
  end
end

# welcome_data = []
# Array.new(10).each_index{|i| welcome_data << {:id => i, :name => i.to_s}}
# c.run(r.db('').Welcome.insert(welcome_data).query)

# # c.run(r.db('').Welcome).length
# # c.run(r.add(1,2))
# # c.run(r.let([['a', r.add(1,2)],
# #              ['b', r.add(:$a, 3)]],
# #             r.add(:$a, :$b)))
# c.run(r.add(1,2))


