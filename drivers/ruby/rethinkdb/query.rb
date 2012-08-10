module RethinkDB
  class RQL_Query #Sexp
    def clean_lst lst
      case lst.class.hash
      when Array.hash               then lst.map{|z| clean_lst(z)}
      when RQL_Query.hash, Tbl.hash then lst.sexp
                                    else lst
      end
    end
    def initialize(init_body); @body = init_body; end
    def sexp; clean_lst @body; end
    def as _class; RQL_Protob.comp(_class, sexp); end
    def query; RQL_Protob.query sexp; end

    def [](ind); self.send(:getattr, ind); end
    def getbykey(attr, key)
      throw "getbykey must be called on a table" if @body[0] != :table
      S._ [:getbykey, @body[1..3], attr, RQL.expr(key)]
    end

    def proc_args(m, proc)
      args = Array.new(C.arity[m] || 0).map{S.gensym}
      args + [proc.call(*(args.map{|x| RQL.var x}))]
    end
    def expand_procs(m, args)
      args.map{|arg| arg.class == Proc ? proc_args(m, arg) : arg}
    end
    def expand_procs_inline(m, args)
      args.map{|arg| arg.class == Proc ? proc_args(m, arg) : [arg]}.flatten(1)
    end

    def connection_send m
      if Connection.last
      then return Connection.last.send(m, self)
      else raise RuntimeError, "No last connection, open a new one."
      end
    end

    #TODO: Arity Checking
    def method_missing(m, *args, &block)
      m = C.method_aliases[m] || m
      if (RQL.methods.include? m.to_s) && (not m.to_s.grep(/.*attrs?/)[0])
        return RQL.send(m, *[@body, *args], &block)
      end
      return self.send(m, *(args + [block])) if block
      if P.enum_type(Builtin::Comparison, m)
        S._ [:call, [:compare, m], [@body, *args]]
      elsif P.enum_type(Builtin::BuiltinType, m)
        args = expand_procs_inline(m, args)
        m_rw = C.query_rewrites[m] || m
        if P.message_field(Builtin, m_rw) then S._ [:call, [m, *args], [@body]]
                                          else S._ [:call, [m], [@body, *args]]
        end
      elsif P.enum_type(WriteQuery::WriteQueryType, m)
        args =(C.repeats.include? m) ? expand_procs_inline(m,args) : expand_procs(m,args)
        if C.repeats.include? m and args[-1].class != Array;  args[-1] = [args[-1]]; end
        S._ [m, @body, *args]
      else super(m, *args, &block)
      end
    end
  end

  module RQL_Mixin
    def getattr a; S._ [:call, [:implicit_getattr, a], []]; end
    def pickattrs *a; S._ [:call, [:implicit_pickattrs, *a], []]; end
    def hasattr a; S._ [:call, [:implicit_hasattr, a], []]; end
    def [](ind); expr ind; end
    def db (x,y=nil); Tbl.new x,y; end
    def expr x
      case x.class().hash
      when RQL_Query.hash  then x
      when String.hash     then S._ [:string, x]
      when Fixnum.hash     then S._ [:number, x]
      when TrueClass.hash  then S._ [:bool, x]
      when FalseClass.hash then S._ [:bool, x]
      when NilClass.hash   then S._ [:json_null]
      when Array.hash      then S._ [:array, *x.map{|y| expr(y)}]
      when Hash.hash       then S._ [:object, *x.map{|var,term| [var, expr(term)]}]
      when Symbol.hash     then S._ x.to_s[0]=='$'[0] ? var(x.to_s[1..-1]) : getattr(x)
      else raise TypeError, "term.expr can't handle '#{x.class()}'"
      end
    end
    def method_missing(m, *args, &block)
      return self.send(C.method_aliases[m], *args, &block) if C.method_aliases[m]
      if    P.enum_type(Builtin::BuiltinType, m) then S._ [:call, [m], args]
      elsif P.enum_type(Builtin::Comparison, m)  then S._ [:call, [:compare, m], args]
      elsif P.enum_type(Term::TermType, m)       then S._ [m, *args]
      else super(m, *args, &block)
      end
    end
  end
  module RQL; extend RQL_Mixin; end

  class Tbl
    def initialize (name, table=nil); @db = name; @table = table; end
    def sexp; [:table, @db, @table]; end
    def method_missing(m, *a, &b)
      if    not @table                 then @table = m; return self
      elsif C.table_directs.include? m then S._([@db, @table]).send(m, *a, &b)
                                       else S._([:table, @db, @table]).send(m, *a, &b)
      end
    end
  end
end
