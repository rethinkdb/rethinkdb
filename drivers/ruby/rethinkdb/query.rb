module RethinkDB
  class RQL_Query #S-expression representation of a query
    def initialize(init_body); @body = init_body; end

    # def ==(rhs)
    #   raise SyntaxError,"
    #   Cannot use inline ==/!= with RQL queries, use .eq() instead.
    #   If you need to see whether two queries are the same, compare
    #   their S-expression representations like: `query1.sexp ==
    #   query2.sexp`."
    # end

    # Turn into an actual S-expression (we wrap it in a class so that
    # instance methods may be invoked).
    def sexp; clean_lst @body; end
    def body; @body; end
    def clean_lst lst
      if    lst.kind_of? RQL_Query then lst.sexp
      elsif lst.class == Array         then lst.map{|z| clean_lst(z)}
      else                                  lst
      end
    end

    # Compile into either a protobuf class or a query.
    def as _class; RQL_Protob.comp(_class, sexp); end
    def query; RQL_Protob.query sexp; end

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

  class Database
    def method_missing(m, *a, &b); self.table(m, *a, &b); end
  end

  class Meta_Query < RQL_Query; end

  class Table
    def method_missing(m, *args, &block)
      Multi_Row_Selection.new(@body).send(m, *args, &block);
    end

    def to_mrs
      Multi_Row_Selection.new(@body)
    end
  end
end

load 'tables.rb'
load 'reads.rb'
load 'writes.rb'
