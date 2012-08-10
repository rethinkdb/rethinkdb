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

    def connection_send m
      if Connection.last
      then return Connection.last.send(m, self)
      else raise RuntimeError, "No last connection, open a new one."
      end
    end

    def method_missing(m, *args, &block)
      m = C.method_aliases[m] || m
      return RQL.send(m, *[self, *args], &block) if RQL.methods.include? m.to_s
      super(m, *args, &block)
    end
  end

  module RQL_Mixin
    def method_missing(m, *args, &block)
      (m2 = C.method_aliases[m]) ? self.send(m2, *args, &block) : super(m, *args, &block)
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
