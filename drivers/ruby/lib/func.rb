module RethinkDB
  class RQL
    @@gensym_mutex = Mutex.new
    @@gensym_cnt = 0
    def new_func(&b)
      args = @@gensym_mutex.synchronize{(0...b.arity).map{@@gensym_cnt += 1}}
      body = b.call(*(args.map{|i| RQL.new.var i}))
      RQL.new.func(args, body)
    end

    # Offsets of the "optarg" optional arguments hash in respective
    # methods.  Some methods change this depending on whether they're
    # passed a block -- they take a hash specifying the offset for
    # each circumstance, instead of an integer.  -1 can be supplied to
    # mean the "last" argument -- whatever argument is specified will
    # only be removed from the argument list and treated as an optarg
    # if it's a Hash.  A positive value is necessary for functions
    # that can take a hash for the last non-optarg argument.
    @@optarg_offsets = {
      :replace => {:with_block => 0, :without => 1},
      :update => {:with_block => 0, :without => 1},
      :insert => 1,
      :delete => -1,
      :reduce => -1,
      :between => 2,
      :table => -1,
      :table_create => -1,
      :reconfigure => 0,
      :get_all => -1,
      :eq_join => -1,
      :javascript => -1,
      :filter => {:with_block => 0, :without => 1},
      :slice => -1,
      :during => -1,
      :orderby => -1,
      :order_by => -1,
      :group => -1,
      :iso8601 => -1,
      :index_create => -1,
      :index_rename => -1,
      :random => -1,
      :http => 1,
      :distinct => -1,
      :distance => -1,
      :circle => -1,
      :get_intersecting => -1,
      :get_nearest => -1,
      :min => -1,
      :max => -1,
      :changes => -1,
      :wait => 0
    }
    @@method_aliases = {
      :lt => :<,
      :le => :<=,
      :gt => :>,
      :ge => :>=,
      :add => :+,
      :sub => :-,
      :mul => :*,
      :div => :/,
      :mod => :%,
      :any => [:"|", :or],
      :all => [:"&", :and],
      :order_by => :orderby,
      :concat_map => :concatmap,
      :for_each => :foreach,
      :javascript => :js,
      :type_of => :typeof
    }

    termtypes = Term::TermType.constants.map{ |c| c.to_sym }

    # r.binary has different behavior when operating on client-side strings vs
    # terms on the server
    termtypes.delete(:BINARY)

    termtypes.each {|termtype|

      method_body = proc { |*a, &b|
        bitop = [:"|", :"&"].include?(__method__)

        if [:<, :<=, :>, :>=, :+, :-, :*, :/, :%].include?(__method__)
          a.each {|arg|
            if arg.class == RQL && arg.bitop
              err = "Calling #{__method__} on result of infix bitwise operator:\n" +
                "#{arg.inspect}.\n" +
                "This is almost always a precedence error.\n" +
                "Note that `a < b | b < c` <==> `a < (b | b) < c`.\n" +
                "If you really want this behavior, use `.or` or `.and` instead."
              raise RqlDriverError, err
            end
          }
        end

        if (opt_offset = @@optarg_offsets[termtype.downcase])
          if opt_offset.class == Hash
            opt_offset = opt_offset[b ? :with_block : :without]
          end
          # TODO: This should drop the Hash comparison or at least
          # @@optarg_offsets should stop specifying -1, where possible.
          # Any time one of these operations is changed to support a
          # hash argument, we'll have to remember to fix
          # @@optarg_offsets, otherwise.
          optargs = a.delete_at(opt_offset) if a[opt_offset].class == Hash
        end

        args = ((@body != RQL) ? [self] : []) + a + (b ? [new_func(&b)] : [])

        t = [Term::TermType.const_get(termtype),
             args.map {|x| RQL.new.expr(x).to_pb},
             *((optargs && optargs != {}) ?
               [Hash[optargs.map {|k,v| [k.to_s, RQL.new.expr(v).to_pb]}]] :
               [])]
        return RQL.new(t, bitop)
      }

      define_method(termtype.downcase, &method_body)

      [*@@method_aliases[termtype.downcase]].each{|method_alias|
        define_method method_alias, &method_body
      }
    }

    def connect(*args, &b)
      unbound_if @body != RQL
      c = Connection.new(*args)
      b ? begin b.call(c) ensure c.close end : c
    end

    def -@; RQL.new.sub(0, self); end

    def [](ind)
      if ind.class == Range
        return slice(ind.begin, ind.end, :right_bound =>
                     (ind.exclude_end? ? 'open' : 'closed'))
      else
        return bracket(ind)
      end
    end

    def ==(rhs)
      raise ArgumentError,"
      Cannot use inline ==/!= with RQL queries, use .eq() instead if
      you want a query that does equality comparison.

      If you need to see whether two queries are the same, compare
      the raw qeuries like: `query1.to_pb == query2.to_pb`."
    end

    def do(*args, &b)
      a = ((@body != RQL) ? [self] : []) + args.dup
      if a == [] && !b
        raise RqlDriverError, "Expected 1 or more arguments but found 0."
      end
      funcall_args = (b ? [new_func(&b)] : [a.pop]) + a
      # PP.pp funcall_args
      RQL.new.funcall(*funcall_args)
    end

    def row
      unbound_if(@body != RQL)
      raise NoMethodError, ("Sorry, r.row is not available in the ruby driver.  " +
                            "Use blocks instead.")
    end
  end
end
