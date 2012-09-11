module RethinkDB
  # An RQL query that can be sent to the RethinkDB cluster.  This class contains
  # only functions that work for any type of query.  Queries are either
  # constructed by methods in the RQL module, or by invoking the instance
  # methods of a query on other queries.
  class RQL_Query
    # Run the invoking query using the most recently opened connection.  See
    # Connection#run for more details.
    def run
      Connection.last.send(:run, self)
    end

    def initialize(init_body) # :nodoc:
      @body = init_body
    end

    def ==(rhs) # :nodoc:
      raise SyntaxError,"
      Cannot use inline ==/!= with RQL queries, use .eq() instead if
      you want a query that does equality comparison.

      If you need to see whether two queries are the same, compare
      their S-expression representations like: `query1.sexp ==
      query2.sexp`."
    end

    # Return the S-expression representation of a query.  Used for
    # equality comparison and advanced debugging.
    def sexp
      clean_lst @body
    end
    def body # :nodoc:
      @body
    end
    def clean_lst lst # :nodoc:
      if    lst.kind_of? RQL_Query     then lst.sexp
      elsif lst.class == Array         then lst.map{|z| clean_lst(z)}
      else                                  lst
      end
    end

    # Compile into either a protobuf class or a query.
    def as _class # :nodoc:
      RQL_Protob.comp(_class, sexp)
    end
    def query # :nodoc:
      RQL_Protob.query sexp
    end

    # Dereference aliases (see utils.rb) and possibly dispatch to RQL
    # (because the caller may be trying to use the more convenient
    # inline version of an RQL function).
    def method_missing(m, *args, &block) # :nodoc:
      if (m2 = C.method_aliases[m]); then return self.send(m2, *args, &block); end
      return RQL.send(m, *[self, *args], &block) if RQL.methods.include? m.to_s
      super(m, *args, &block)
    end
  end
end

load 'tables.rb'
load 'sequence.rb'
load 'streams.rb'
load 'jsons.rb'
load 'writes.rb'
