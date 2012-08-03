require 'rubygems'
require 'query_language.pb.rb'
require 'socket'
require 'pp'

require 'utils.rb'
require 'protob_compiler.rb'

module RethinkDB
  class S #Sexp
    def clean_lst lst
      case lst.class.hash
      when Array.hash then lst.map{|z| clean_lst(z)}
      when S.hash     then lst.sexp
      else lst
      end
    end
    def initialize(init_body); @body = init_body; end
    def sexp; clean_lst @body; end
    def protob; RQL_Protob.comp(Term, sexp); end

    def [](ind); S.new [:call, [:getattr, ind.to_s], [@body]]; end
    def getbykey(attr, key)
      throw "getbykey must be called on a table" if @body[0] != :table
      S.new [:getbykey, @body[1..3], attr, RQL.expr(key)]
    end

    def method_missing(m, *args, &block)
      m = C.method_aliases[m] || m
      if P.enum_type(Builtin::Comparison, m)
        S.new [:call, [:compare, m], [@body, *args]]
      elsif P.enum_type(Builtin::BuiltinType, m)
        if P.message_field(Builtin, m) then S.new [:call, [m, *args], [@body]]
        else S.new [:call, [m], [@body, *args]]
        end
      else super(m, *args, &block)
      end
    end
  end

  module RQL_Mixin
    def attr a; S.new [:call, [:implicit_getattr, a], []]; end
    def expr x
      case x.class().hash
      when S.hash          then x
      when String.hash     then S.new [:string, x]
      when Fixnum.hash     then S.new [:number, x]
      when TrueClass.hash  then S.new [:bool, x]
      when FalseClass.hash then S.new [:bool, x]
      when NilClass.hash   then S.new [:json_null]
      when Array.hash      then S.new [:array, *x.map{|y| expr(y)}]
      when Hash.hash       then S.new [:object, *x.map{|var,term| [var, expr(term)]}]
      when Symbol.hash     then
        str = x.to_s
        S.new str[0] == '$'[0] ? var(str[1..str.length]) : attr(str)
      else raise TypeError, "term.expr can't handle type `#{x.class()}`"
      end
    end
    def method_missing(m, *args, &block)
      m = C.method_aliases[m] || m
      if    P.enum_type(Builtin::BuiltinType, m) then S.new [:call, [m], args]
      elsif P.enum_type(Builtin::Comparison, m)  then S.new [:call, [:compare, m], args]
      elsif P.enum_type(Term::TermType, m)       then S.new [m, *args]
      else  super(m, *args, &block)
      end
    end
  end
  module RQL; extend RQL_Mixin; end
end
