module RethinkDB
  class RQL_Query #S-expression representation of a query
    def initialize(init_body); @body = init_body; end

    # Turn into an actual S-expression (we wrap it in a class so that
    # instance methods may be invoked).
    def sexp; clean_lst @body; end
    def body; @body; end
    def clean_lst lst
      case lst.class.hash
      when Array.hash               then lst.map{|z| clean_lst(z)}
      when RQL_Query.hash, Tbl.hash then lst.sexp
                                    else lst
      end
    end

    # Compile into either a protobuf class or a query.
    def as _class; RQL_Protob.comp(_class, sexp); end
    def query; RQL_Protob.query sexp; end

    # Sed a message to the last connection with `self` as an argument.
    def connection_send m
      if Connection.last
      then return Connection.last.send(m, self)
      else raise RuntimeError, "No last connection, open a new one."
      end
    end

    # Dereference aliases (see utils.rb) and possibly dispatch to RQL
    # (because the caller may be trying to use the more convenient
    # inline version of an RQL function).
    def method_missing(m, *args, &block)
      if (m2 = C.method_aliases[m]); then return self.send(m2, *args, &block); end
      return RQL.send(m, *[self, *args], &block) if RQL.methods.include? m.to_s
      super(m, *args, &block)
    end
  end

  module RQL_Mixin
    # Dereference aliases (seet utils.rb)
    def method_missing(m, *args, &block)
      (m2 = C.method_aliases[m]) ? self.send(m2, *args, &block) : super(m, *args, &block)
    end
  end
  module RQL; extend RQL_Mixin; end

  # Created by r.db('').Welcome or r.db('','Welcome').  Sort of
  # complicated because sometimes it needs to be a table term, while
  # other times it need to be a tableref, and we also need to support
  # the .Welcome syntax for compatability with the Python client.
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
