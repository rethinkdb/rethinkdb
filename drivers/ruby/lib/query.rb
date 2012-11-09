# Copyright 2010-2012 RethinkDB, all rights reserved.
module RethinkDB
  # An RQL query that can be sent to the RethinkDB cluster.  This class contains
  # only functions that work for any type of query.  Queries are either
  # constructed by methods in the RQL module, or by invoking the instance
  # methods of a query on other queries.
  class RQL_Query
    def boolop? # :nodoc:
      false
    end
    # Run the invoking query using the most recently opened connection.  See
    # Connection#run for more details.
    def run(*args)
      Connection.last_connection.send(:run, self, *args)
    end

    def set_body(val, context=nil) # :nodoc:
      @context = context || caller
      @body = val
    end

    def initialize(init_body, context=nil) # :nodoc:
      set_body(init_body, context)
    end

    def ==(rhs) # :nodoc:
      raise ArgumentError,"
      Cannot use inline ==/!= with RQL queries, use .eq() instead if
      you want a query that does equality comparison.

      If you need to see whether two queries are the same, compare
      their S-expression representations like: `query1.sexp ==
      query2.sexp`."
    end

    # Return the S-expression representation of a query.  Used for
    # equality comparison and advanced debugging.
    def sexp
      S.clean_lst @body
    end
    attr_accessor :body, :marked, :context

    # Compile into either a protobuf class or a query.
    def as _class # :nodoc:
      RQL_Protob.comp(_class, sexp)
    end
    def query(map={S.conn_outdated => false}) # :nodoc:
      RQL_Protob.query(S.replace(sexp, map))
    end

    def print_backtrace(bt) # :nodoc:
      #PP.pp [bt, bt.map{|x| BT.expand x}.flatten, sexp]
      begin
        BT.with_marked_error(self, bt) {
          query = "Query: #{inspect}\n       #{BT.with_highlight {inspect}}"
          line = "Line: #{BT.line || "Unknown"}"
          line + "\n" + query
        }
      rescue Exception => e
        PP.pp [bt, e] if $DEBUG
        "<Internal error in query pretty printer.>"
      end
    end

    def pprint # :nodoc:
      return @body.inspect if @body.class != Array
      return "" if @body == []
      case @body[0]
      when :call then
        if @body[2].length == 1
          @body[2][0].inspect + "." + RQL_Query.new(@body[1],@context).inspect
        else
          func = @body[1][0].to_s
          func_args = @body[1][1..-1].map{|x| x.inspect}
          call_args = @body[2].map{|x| x.inspect}
          func + "(" + (func_args + call_args).join(", ") + ")"
        end
      else
        func = @body[0].to_s
        func_args = @body[1..-1].map{|x| x.inspect}
        func + "(" + func_args.join(", ") + ")"
      end
    end

    def real_inspect(args) # :nodoc:
      str = args[:str] || pprint
      (BT.highlight and @marked) ? "\000"*str.length : str
    end

    def inspect # :nodoc:
      real_inspect({})
    end

    # Dereference aliases (see utils.rb) and possibly dispatch to RQL
    # (because the caller may be trying to use the more convenient
    # inline version of an RQL function).
    def method_missing(m, *args, &block) # :nodoc:
      if (m2 = C.method_aliases[m]); then return self.send(m2, *args, &block); end
      if RQL.methods.map{|x| x.to_sym}.include?(m)
        return RQL.send(m, *[self, *args], &block)
      end
      super(m, *args, &block)
    end
  end
end

load 'tables.rb'
load 'sequence.rb'
load 'streams.rb'
load 'jsons.rb'
load 'writes.rb'
