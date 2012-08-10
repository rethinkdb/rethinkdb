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
      return RQL.send(m, *[self, *args], &block) if RQL.methods.include? m.to_s
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
    def method_missing(m, *args, &block)
      return self.send(C.method_aliases[m], *args, &block) if C.method_aliases[m]
      super(m, *args, &block)
    end
  end
  module RQL; extend RQL_Mixin; end

  class Tbl
    def initialize (name, table=nil); @db = name; @table = table; end
    def sexp; [:table, @db, @table]; end
    def method_missing(m, *a, &b)
      if    not @table                 then @table = m; return self
      elsif m == :expr                 then S._ [:table, @db, @table]
      elsif C.table_directs.include? m then S._([@db, @table]).send(m, *a, &b)
                                       else S._([:table, @db, @table]).send(m, *a, &b)
      end
    end
  end
end
