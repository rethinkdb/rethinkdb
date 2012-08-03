require 'rubygems'
require 'query_language.pb.rb'
require 'socket'
require 'pp'
load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/parse.rb'

class ToTerm
  def _ x; SexpTerm.new x; end
  def attr a; _ [:call, [:implicit_getattr, a], []]; end
  def expr x
    return x.body if x.class() == SexpTerm
    case x.class().hash
    when SexpTerm.hash   then x
    when String.hash     then _ [:string, x]
    when Fixnum.hash     then _ [:number, x]
    when TrueClass.hash  then _ [:bool, x]
    when FalseClass.hash then _ [:bool, x]
    when NilClass.hash   then _ [:json_null]
    when Array.hash      then _ [:array, *x.map{|y| expr(y)}]
    when Hash.hash       then _ [:object, *x.map{|var,term| [var, expr(term)]}]
    else raise TypeError, "term.expr can't handle type `#{x.class()}`"
    end
  end
  def method_missing(meth, *args, &block)
    if enum_type(Builtin::BuiltinType, meth)
      _ [:call, [meth], args]
    else
      _ [meth, *args]
    end
  end
end
$reql = ToTerm.new

def clean_lst lst
  case lst.class.hash
  when Array.hash then lst.map{|z| clean_lst(z)}
  when SexpTerm.hash then clean_lst lst.body
  else lst
  end
end

class SexpTerm
  def initialize(init_body); @body = init_body; end
  def sexp; clean_lst @body; end
  def protob; parse(Term, sexp); end

  def [](ind); SexpTerm.new [:call, [:getattr, ind], [@body]]; end
  def getbykey(attr, key)
    throw "getbykey must be called on a table" if body[0] != :table
    SexpTerm.new [:getbykey, @body[1..3], attr, $reql.expr(key)]
  end

  def method_missing(meth, *args, &block)
    if enum_type(Builtin::Comparison, meth)
      sexp_args = args.map {|arg| $reql.expr(arg)}
      SexpTerm.new [:call, [:compare, meth], [@body, *sexp_args]]
    else
      SexpTerm.new [:call, [meth, *args], [@body]]
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

reql.all(reql.var('a'), reql.var('b')).protob
# term.var('x').protob
# PP.pp(term.let([["a", term.expr(1)],
#                 ["b", term.expr(2)]],
#                term.var('a').eq(1).and(term.var.('b').eq(2))).protob)
# PP.pp(term.table('a','b').filter('row', term.var('@')['a'].eq(5)).protob)
# PP.pp(term.expr({ 'a' => 5, 'b' => 'foo' }).protob)
# PP.pp(term.table({:db_name => 'a',:table_name => 'b'}).filter('row', term.var('@')['a'].eq(5)).protob)
reql.var('x').protob #VAR
reql.let([["a", 1], #LET
          ["b", 2]],
         reql.all(reql.var('a').eq(1), reql.var('b').eq(2))).protob
#CALL not done explicitly
reql.if(reql.var('a'), 1, '2').protob #IF
reql.error('e').protob #ERROR
reql.expr(5).protob #NUMBER
reql.expr('s').protob #STRING
reql.json('{type : "json"}').protob #JSON
reql.expr(true).protob #BOOL
reql.expr(false).protob #BOOL
reql.expr(nil).protob #JSON_NULL
reql.expr([1, 2, [4, 5]]).protob #ARRAY
reql.expr({'a' => 1, 'b' => '2', 'c' => { 'nested' => true }}).protob #OBJECT
reql.table('a', 'b').getbykey('attr', 5).protob #GETBYKEY
reql.table('a', 'b').protob #TABLE
reql.table({:table_name => 'b'}).protob #TABLE
reql.javascript('javascript').protob #JAVASCRIPT






