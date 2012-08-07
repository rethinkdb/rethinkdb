require 'rubygems'
require 'query_language.pb.rb'
require 'socket'
require 'pp'

load 'utils.rb'
load 'protob_compiler.rb'

module RethinkDB
  class S #Sexp
    @@gensym_counter = 0
    def gensym; 'gensym_'+(@@gensym_counter += 1).to_s; end
    def clean_lst lst
      case lst.class.hash
      when Array.hash then lst.map{|z| clean_lst(z)}
      when S.hash, Tbl.hash then lst.sexp
      else lst
      end
    end
    def initialize(init_body); @body = init_body; end
    def sexp; clean_lst @body; end
    def as _class; RQL_Protob.comp(_class, sexp); end
    def query;  RQL_Protob.query sexp; end

    def [](ind); self.send(:getattr, ind); end
    def getbykey(attr, key)
      throw "getbykey must be called on a table" if @body[0] != :table
      S.new [:getbykey, @body[1..3], attr, RQL.expr(key)]
    end

    def get_proc_args(proc)
      args = Array.new([0, proc.arity].max).map{gensym}
      args + [proc.call(*(args.map{|x| RQL.var x}))]
    end
    def expand_procs(args)
      args.map{|arg| arg.class == Proc ? get_proc_args(arg) : arg}
    end
    def expand_procs_inline(args)
      args.map{|arg| arg.class == Proc ? get_proc_args(arg) : [arg]}.flatten(1)
    end

    #TODO: Arity Checking
    def method_missing(m, *args, &block)
      return self.send(m, *(args + [block])) if block
      m = C.method_aliases[m] || m
      if P.enum_type(Builtin::Comparison, m)
        S.new [:call, [:compare, m], [@body, *args]]
      elsif P.enum_type(Builtin::BuiltinType, m)
        args = expand_procs_inline(args)
        m_rw = C.query_rewrites[m] || m
        if P.message_field(Builtin, m_rw) then S.new [:call, [m, *args], [@body]]
                                          else S.new [:call, [m], [@body, *args]]
        end
      elsif P.enum_type(WriteQuery::WriteQueryType, m)
        args = (C.repeats.include? m) ? expand_procs_inline(args) : expand_procs(args)
        if C.repeats.include? m and args[-1].class != Array;  args[-1] = [args[-1]]; end
        #PP.pp P.message_field(WriteQuery, m)
        if P.message_field(WriteQuery, m)
          #PP.pp P.message_field(WriteQuery, m)[1].type
        end
        S.new [m, @body, *args]
      else super(m, *args, &block)
      end
    end
  end

  module RQL_Mixin
    #TODO: All Aliases
    def attr a; S.new [:call, [:implicit_getattr, a], []]; end
    def attrs *a; S.new [:call, [:implicit_pickattrs, *a], []]; end
    def attr? a; S.new [:call, [:implicit_hasattr, a], []]; end
    def db x; Tbl.new x; end
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
        S.new str[0] == '$'[0] ? var(str[1..-1]) : attr(str)
      else raise TypeError, "term.expr can't handle type '#{x.class()}'"
      end
    end
    def method_missing(m, *args, &block)
      m = C.method_aliases[m] || m
      if    P.enum_type(Builtin::BuiltinType, m) then S.new [:call, [m], args]
      elsif P.enum_type(Builtin::Comparison, m)  then S.new [:call, [:compare, m], args]
      elsif P.enum_type(Term::TermType, m)       then S.new [m, *args]
                                                 else super(m, *args, &block)
      end
    end
  end
  module RQL; extend RQL_Mixin; end

  class Tbl
    def initialize (name); @db = name; @table = nil; end
    def sexp; [:table, @db, @table]; end
    def method_missing(m, *a, &b)
      if    not @table                 then @table = m; return self
      elsif C.table_directs.include? m then S.new([@db, @table]).send(m, *a, &b)
                                       else S.new([:table, @db, @table]).send(m, *a, &b)
      end
    end
  end
end
