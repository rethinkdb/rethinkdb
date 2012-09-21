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

    def set_body(val, context=nil) # :nodoc:
      @context = context || caller
      @body = val
    end

    def initialize(init_body, context=nil) # :nodoc:
      set_body(init_body, context)
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
      S.clean_lst @body
    end
    def body # :nodoc:
      @body
    end

    # Compile into either a protobuf class or a query.
    def as _class # :nodoc:
      RQL_Protob.comp(_class, sexp)
    end
    def query(*args) # :nodoc:
      RQL_Protob.query(args == [] ? sexp : S.replace(sexp, *args))
    end

    def print_backtrace(bt)
      B.line = nil
      query = inspect
      highlights = B.format_highlights(inspect{{:bt => bt, :highlight => true}})
      line = "Line: #{B.line || "Unknown"}"
      "#{line}\nQuery: #{query}\n       #{highlights}\n"
    end

    def pprint(args)
      elt_bt = other_bt = [:noprint]
      if args[:bt] != :noprint
        if args[:bt]
          elt = args[:bt][0]
          elt_bt = args[:bt][1..-1]
        end
      end
      elt_b = lambda {{:highlight => args[:highlight], :bt => elt_bt}}
      other_b = lambda {{:highlight => args[:highlight], :bt => other_bt}}
      b = other_b

      return @body.inspect(&b) if @body.class != Array
      return "" if @body == []
      case @body[0]
      when :call then
        if @body[2].length == 1
          @body[2][0].inspect(&b) + "." + RQL_Query.new(@body[1],@context).inspect(&b)
        else
          func = @body[1][0].to_s
          func_args = @body[1][1..-1].map{|x| x.inspect(&other_b)}
          pre_pb_args = @body[2]
          ind = (elt =~ /arg:([0-9]+)/) ? $1.to_i : -1
          pb_args = []
          pre_pb_args.each_index{|i|
            this_b = (i == ind) ? elt_b : other_b
            pb_args.push pre_pb_args[i].inspect(&this_b)
          }

          func + "(" + (func_args + pb_args).join(", ") + ")"
        end
      else
        @body[0].to_s + "(" + @body[1..-1].map{|x| x.inspect(&b)}.join(", ") + ")"
      end
    end

    def real_inspect(args, &b)
      args = b.call.merge(args) if b
      str = args[:str] || pprint(args)
      if args[:bt] == []
        B.line = B.sanitize_context(@context)[0] if !B.line
        str = B.maybe_highlight(str, args)
      end
      return str
    end

    def inspect(&block)
      real_inspect(block ? yield : {})
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
