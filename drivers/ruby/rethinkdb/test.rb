load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/rethinkdb.rb'
r = RethinkDB::RQL
################################################################################
#                                  TERM TESTS                                  #
################################################################################
def p? x; x; end
r.var('x').protob ;p? :VAR
r.expr(:$x).protob ;p? :VAR
r.let([["a", 1],
       ["b", 2]],
      r.all(r.var('a').eq(1), r.var('b').eq(2))).protob ;p? :LET
#CALL not done explicitly
r.if(r.var('a'), 1, '2').protob ;p? :IF
r.error('e').protob ;p? :ERROR
r.expr(5).protob ;p? :NUMBER
r.expr('s').protob ;p? :STRING
r.json('{type : "json"}').protob ;p? :JSON
r.expr(true).protob ;p? :BOOL
r.expr(false).protob ;p? :BOOL
r.expr(nil).protob ;p? :JSON_NULL
r.expr([1, 2, [4, 5]]).protob ;p? :ARRAY
r.expr({'a' => 1, 'b' => '2', 'c' => { 'nested' => true }}).protob ;p? :OBJECT
r.table('a', 'b').getbykey('attr', 5).protob ;p? :GETBYKEY
r.table('a', 'b').protob ;p? :TABLE
r.table({:table_name => 'b'}).protob ;p? :TABLE
r.javascript('javascript').protob ;p? :JAVASCRIPT

################################################################################
#                                 BUILTIN TESTS                                #
################################################################################
r.not(true).protob ;p? :NOT
r.expr(true).not.protob ;p? :NOT
r.var('v')['a'].protob ;p? :GETATTR
r.var('v')[:a].protob ;p? :GETATTR
r.attr('a').protob ;p? :IMPLICIT_GETATTR
r.expr(:a).protob ;p? :IMPLICIT_GETATTR
#HASATTR
#IMPLICIT_HASATTR
#PICKATTRS
#IMPLICIT_PICKATTRS
#MAPMERGE
#ARRAYAPPEND
#ARRAYCONCAT
#ARRAYSLICE
#ARRAYNTH
#ARRAYLENGTH
r.add(1, 2).protob ;p? :ADD
r.expr(1).add(2).protob ;p? :ADD
(r.expr(1) + 2).protob ;p? :ADD
r.subtract(1, 2).protob ;p? :SUBTRACT
r.expr(1).subtract(2).protob ;p? :SUBTRACT
(r.expr(1) - 2).protob ;p? :SUBTRACT
r.multiply(1, 2).protob ;p? :MULTIPLY
r.expr(1).multiply(2).protob ;p? :MULTIPLY
(r.expr(1) * 2).protob ;p? :MULTIPLY
r.divide(1, 2).protob ;p? :DIVIDE
r.expr(1).divide(2).protob ;p? :DIVIDE
(r.expr(1) / 2).protob ;p? :DIVIDE
r.modulo(1, 2).protob ;p? :MODULO
r.expr(1).modulo(2).protob ;p? :MODULO
(r.expr(1) % 2).protob ;p? :MODULO

r.eq(1, 2).protob ;p? :COMPARE_EQ
r.expr(1).eq(2).protob ;p? :COMPARE_EQ
r.equals(1, 2).protob ;p? :COMPARE_EQ
r.expr(1).equals(2).protob ;p? :COMPARE_EQ

r.ne(1, 2).protob ;p? :COMPARE_NE
r.expr(1).ne(2).protob ;p? :COMPARE_NE
r.neq(1, 2).protob ;p? :COMPARE_NE
r.expr(1).neq(2).protob ;p? :COMPARE_NE

r.lt(1, 2).protob ;p? :COMPARE_LT
r.expr(1).lt(2).protob ;p? :COMPARE_LT
(r.expr(1) < 2).protob ;p? :COMPARE_LT

r.le(1, 2).protob ;p? :COMPARE_LE
r.expr(1).le(2).protob ;p? :COMPARE_LE
(r.expr(1) <= 2).protob ;p? :COMPARE_LE

r.gt(1, 2).protob ;p? :COMPARE_GT
r.expr(1).gt(2).protob ;p? :COMPARE_GT
(r.expr(1) > 2).protob ;p? :COMPARE_GT

r.ge(1, 2).protob ;p? :COMPARE_GE
r.expr(1).ge(2).protob ;p? :COMPARE_GE
(r.expr(1) >= 2).protob ;p? :COMPARE_GE

r.table('','Welcome').filter('row', r.eq(:id, 1)).protob ;p? :FILTER
r.table('','Welcome').filter { r.eq(:id, 1) }.protob ;p? :FILTER
r.table('','Welcome').filter { |row| row[:id].eq(1) }.protob ;p? :FILTER
r.table('','Welcome').map('row', r.expr(:$row)).protob ;p? :MAP
r.table('','Welcome').concatmap('row',
    r.table('','Other').filter('row2',
        r.var('row')[:id] > :id)).protob ;p? :CONCATMAP
r.table('','Welcome').concatmap { |row|
  r.table('','Other').filter {
    row[:id] > :id
  }
}.protob ;p? :CONCATMAP

#ORDERBY
#DISTINCT
#LIMIT
#LENGTH
#UNION
#NTH
#STREAMTOARRAY
#ARRAYTOSTREAM
#REDUCE
#GROUPEDMAPREDUCE
#ANY
#ALL
#RANGE
#SKIP
