require 'rubygems'
require 'query_language.pb.rb'
require 'socket'
require 'pp'
load '/home/mlucy/rethinkdb_ruby/drivers/ruby/rethinkdb/parse.rb'

class ToTerm
  def _ x; SexpTerm.new x; end
  def attr a; _ [:call, [:implicit_getattr, a], []]; end
  def native x
    return x.body if x.class() == SexpTerm
    case x.class().hash
    when SexpTerm.hash   then x
    when String.hash     then _ [:string, x]
    when Fixnum.hash     then _ [:number, x]
    when TrueClass.hash  then _ [:bool, x]
    when FalseClass.hash then _ [:bool, x]
    when NilClass.hash   then _ [:json_null]
    when Array.hash      then _ [:array, *x.map{|y| native(y)}]
    when Hash.hash       then _ [:object, *x.map{|var,term| [[var], native(term)]}]
    else raise TypeError, "term.native can't handle type `#{x.class()}`"
    end
  end
  def method_missing(meth, *args, &block); _ [meth, *args]; end
end
term = ToTerm.new

def clean_lst lst
  case lst.class.hash
  when Array.hash then lst.map{|z| clean_lst(z)}
  when SexpTerm.hash then clean_lst lst.body
  else lst
  end
end

class SexpTerm
  def initialize(init_body); @body = init_body; @term = ToTerm.new; end
  def sexp; clean_lst @body; end
  def protob; parse(Term, sexp); end

  def [](ind); SexpTerm.new [:call, [:getattr, ind], [@body]]; end

  def method_missing(meth, *args, &block)
    if enum_type(Builtin::Comparison, meth)
      sexp_args = args.map {|arg| @term.native(arg)}
      SexpTerm.new [:call, [:compare, meth], [@body, *sexp_args]]
    else
      SexpTerm.new [:call, [meth, *args], [@body]]
    end
  end
end

PP.pp(parse(Term,
      [:call,
       [:filter,
        "row",
        [:call,
         [:all],
         [[:call,
           [:compare, :eq],
           [[:call, [:implicit_getattr, "a"], []],
            [:number, 0]]]]]],
       [[:table, "", "Welcome"]]]))

PP.pp(term.native({ 'a' => 5, 'b' => 'foo' }).protob)
PP.pp(term.table('a','b').filter('row', term.var('@')['a'].eq(5)).protob)
term.let([["a", term.native(1)], ["b", term.native(2)]], term.var('a').eq(5)).protob
