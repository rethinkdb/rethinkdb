require 'rubygems'
require 'query_language.pb.rb'
require 'socket'
require 'pp'
load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/parse.rb'

def _ x; SexpTerm.new x; end
class Reql
  def attr a; _ [:call, [:implicit_getattr, a], []]; end
  def expr x
    return x.sexp if x.class() == SexpTerm
    case x.class().hash
    when SexpTerm.hash   then x
    when String.hash     then _ [:string, x]
    when Fixnum.hash     then _ [:number, x]
    when TrueClass.hash  then _ [:bool, x]
    when FalseClass.hash then _ [:bool, x]
    when NilClass.hash   then _ [:json_null]
    when Array.hash      then _ [:array, *x.map{|y| expr(y)}]
    when Hash.hash       then _ [:object, *x.map{|var,term| [var, expr(term)]}]
    when Symbol.hash     then
      str = x.to_s
      _ str[0] == '$'[0] ? var(str[1..str.length]) : attr(str)
    else raise TypeError, "term.expr can't handle type `#{x.class()}`"
    end
  end
  def method_missing(meth, *args, &block)
    meth = $method_aliases[meth] || meth
    case
    when enum_type(Builtin::BuiltinType, meth) then _ [:call, [meth], args]
    when enum_type(Builtin::Comparison, meth)  then _ [:call, [:compare, meth], args]
    when enum_type(Term::TermType, meth)       then _ [meth, *args]
    else super(meth, *args, &block)
    end
  end
end
$reql = Reql.new

def clean_lst lst
  case lst.class.hash
  when Array.hash then lst.map{|z| clean_lst(z)}
  when SexpTerm.hash then lst.sexp
  else lst
  end
end

$method_aliases = {
  :equals => :eq, :neq => :neq,
  :neq => :ne, :< => :lt, :<= => :le, :> => :gt, :>= => :ge,
  :+ => :add, :- => :subtract, :* => :multiply, :/ => :divide, :% => :modulo
}
class SexpTerm
  def _ x; SexpTerm.new x; end
  def initialize(init_body); @body = init_body; end
  def sexp; clean_lst @body; end
  def protob; parse(Term, sexp); end

  def [](ind); _ [:call, [:getattr, ind.to_s], [@body]]; end
  def getbykey(attr, key)
    throw "getbykey must be called on a table" if @body[0] != :table
    _ [:getbykey, @body[1..3], attr, $reql.expr(key)]
  end

  def method_missing(meth, *args, &block)
    meth = $method_aliases[meth] || meth
    if enum_type(Builtin::Comparison, meth)
      _ [:call, [:compare, meth], [@body, *args]]
    elsif enum_type(Builtin::BuiltinType, meth)
      if message_field(Builtin, meth) then _ [:call, [meth, *args], [@body]]
      else _ [:call, [meth], [@body, *args]]
      end
    else super(meth, *args, &block)
    end
  end
end

# PP.pp(parse(Term,
#       [:call,
#        [:filter,
#         "row",
#         [:call,
#          [:all],
#          [[:call,
#            [:compare, :eq],
#            [[:call, [:implicit_getattr, "a"], []],
#             [:number, 0]]]]]],
#        [[:table, "", "Welcome"]]]))

$reql.all($reql.var('a'), $reql.var('b')).protob
# term.var('x').protob
# PP.pp(term.let([["a", term.expr(1)],
#                 ["b", term.expr(2)]],
#                term.var('a').eq(1).and(term.var.('b').eq(2))).protob)
# PP.pp(term.table('a','b').filter('row', term.var('@')['a'].eq(5)).protob)
# PP.pp(term.expr({ 'a' => 5, 'b' => 'foo' }).protob)
# PP.pp(term.table({:db_name => 'a',:table_name => 'b'}).filter('row', term.var('@')['a'].eq(5)).protob)

################################################################################
#                                  TERM TESTS                                  #
################################################################################
def p? x; x; end
$reql.var('x').protob ;p? :VAR
$reql.expr(:$x).protob ;p? :VAR
$reql.let([["a", 1],
          ["b", 2]],
          $reql.all($reql.var('a').eq(1), $reql.var('b').eq(2))).protob ;p? :LET
#CALL not done explicitly
$reql.if($reql.var('a'), 1, '2').protob ;p? :IF
$reql.error('e').protob ;p? :ERROR
$reql.expr(5).protob ;p? :NUMBER
$reql.expr('s').protob ;p? :STRING
$reql.json('{type : "json"}').protob ;p? :JSON
$reql.expr(true).protob ;p? :BOOL
$reql.expr(false).protob ;p? :BOOL
$reql.expr(nil).protob ;p? :JSON_NULL
$reql.expr([1, 2, [4, 5]]).protob ;p? :ARRAY
$reql.expr({'a' => 1, 'b' => '2', 'c' => { 'nested' => true }}).protob ;p? :OBJECT
$reql.table('a', 'b').getbykey('attr', 5).protob ;p? :GETBYKEY
$reql.table('a', 'b').protob ;p? :TABLE
$reql.table({:table_name => 'b'}).protob ;p? :TABLE
$reql.javascript('javascript').protob ;p? :JAVASCRIPT

################################################################################
#                                 BUILTIN TESTS                                #
################################################################################
$reql.not(true).protob ;p? :NOT
$reql.expr(true).not.protob ;p? :NOT
$reql.var('v')['a'].protob ;p? :GETATTR
$reql.var('v')[:a].protob ;p? :GETATTR
$reql.attr('a').protob ;p? :IMPLICIT_GETATTR
$reql.expr(:a).protob ;p? :IMPLICIT_GETATTR
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
$reql.add(1, 2).protob ;p? :ADD
$reql.expr(1).add(2).protob ;p? :ADD
($reql.expr(1) + 2).protob ;p? :ADD
$reql.subtract(1, 2).protob ;p? :SUBTRACT
$reql.expr(1).subtract(2).protob ;p? :SUBTRACT
($reql.expr(1) - 2).protob ;p? :SUBTRACT
$reql.multiply(1, 2).protob ;p? :MULTIPLY
$reql.expr(1).multiply(2).protob ;p? :MULTIPLY
($reql.expr(1) * 2).protob ;p? :MULTIPLY
$reql.divide(1, 2).protob ;p? :DIVIDE
$reql.expr(1).divide(2).protob ;p? :DIVIDE
($reql.expr(1) / 2).protob ;p? :DIVIDE
$reql.modulo(1, 2).protob ;p? :MODULO
$reql.expr(1).modulo(2).protob ;p? :MODULO
($reql.expr(1) % 2).protob ;p? :MODULO

$reql.eq(1, 2).protob ;p? :COMPARE_EQ
$reql.expr(1).eq(2).protob ;p? :COMPARE_EQ
$reql.equals(1, 2).protob ;p? :COMPARE_EQ
$reql.expr(1).equals(2).protob ;p? :COMPARE_EQ

$reql.ne(1, 2).protob ;p? :COMPARE_NE
$reql.expr(1).ne(2).protob ;p? :COMPARE_NE
$reql.neq(1, 2).protob ;p? :COMPARE_NE
$reql.expr(1).neq(2).protob ;p? :COMPARE_NE

$reql.lt(1, 2).protob ;p? :COMPARE_LT
$reql.expr(1).lt(2).protob ;p? :COMPARE_LT
($reql.expr(1) < 2).protob ;p? :COMPARE_LT

$reql.le(1, 2).protob ;p? :COMPARE_LE
$reql.expr(1).le(2).protob ;p? :COMPARE_LE
($reql.expr(1) <= 2).protob ;p? :COMPARE_LE

$reql.gt(1, 2).protob ;p? :COMPARE_GT
$reql.expr(1).gt(2).protob ;p? :COMPARE_GT
($reql.expr(1) > 2).protob ;p? :COMPARE_GT

$reql.ge(1, 2).protob ;p? :COMPARE_GE
$reql.expr(1).ge(2).protob ;p? :COMPARE_GE
($reql.expr(1) >= 2).protob ;p? :COMPARE_GE

$reql.table('','Welcome').filter('row', $reql.eq(:id, 1)).protob ;p? :FILTER
$reql.table('','Welcome').map('row', $reql.expr(:$row)).protob ;p? :MAP
$reql.table('','Welcome').concatmap('row',
    $reql.table('','Other').filter('row2',
        $reql.expr(:$row)[:id] > :id)).protob ;p? :CONCATMAP

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

class Tst5
  def method_missing(meth, *args, &block)
    if meth == :tst
      PP.pp([meth, args, block])
      $out = block
      block.call(:block) if block
    else
      super(meth)
    end
  end
end
