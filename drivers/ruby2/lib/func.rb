module RethinkDB
  class RQL
    @@gensym_cnt = 0
    def new_func(&b)
      args = (0...b.arity).map{@@gensym_cnt += 1}
      body = b.call(*(args.map{|i| RQL.new.var i}))
      RQL.new.func(args, body)
    end

    @@special_optargs = {
      :replace => :non_atomic, :update => :non_atomic, :insert => :upsert
    }
    @@opt_off = {
      :reduce => -1, :between => -1, :grouped_map_reduce => -1,
      :table => -1, :table_create => -1
    }
    @@rewrites = {
      :< => :lt, :<= => :le, :> => :gt, :>= => :ge,
      :+ => :add, :- => :sub, :* => :mul, :/ => :div, :% => :mod,
      :"|" => :any, :or => :any,
      :"&" => :all, :and => :all,
      :order_by => :orderby,
      :group_by => :groupby,
      :concat_map => :concatmap,
      :for_each => :foreach
    }
    def method_missing(m, *a, &b)
      m = @@rewrites[m] || m
      termtype = Term2::TermType.values[m.to_s.upcase.to_sym]
      unbound_if(!termtype, m)

      if (opt_name = @@special_optargs[m])
        a = optarg_jiggle(a, opt_name)
        opt_offset = -1
      end
      if (opt_offset ||= @@opt_off[m])
        optargs = a.delete_at(opt_offset) if a[opt_offset].class == Hash
      end

      args = (@body ? [self] : []) + a + (b ? [new_func(&b)] : [])
      t = Term2.new
      t.type = termtype
      t.args = args.map{|x| RQL.new.expr(x).to_pb}
      t.optargs = (optargs || {}).map {|k,v|
        ap = Term2::AssocPair.new
        ap.key = k.to_s
        ap.val = RQL.new.expr(v).to_pb
        ap
      }
      return RQL.new t
    end

    def group_by(*a, &b)
      a = [self] + a if @body
      RQL.new.method_missing(:group_by, a[0], a[1..-2], a[-1], &b)
    end
    def groupby(*a, &b); group_by(*a, &b); end

    def optarg_jiggle(args, optarg)
      if (ind = args.map{|x| x.class == Symbol ? x : nil}.index(optarg))
        args << {args.delete_at(ind) => true}
      else
        args << {}
      end
      return args
    end

    def connect(*args)
      unbound_if @body
      Connection.new(*args)
    end

    def avg(attr)
      unbound_if @body
      {:AVG => attr}
    end
    def sum(attr)
      unbound_if @body
      {:SUM => attr}
    end
    def count(*a, &b)
      !@body && a == [] ? {:COUNT => true} : super(*a, &b)
    end

    def reduce(*a, &b)
      a = a[1..-2] + [{:base => a[-1]}] if a.size + (@body ? 1 : 0) == 2
      super(*a, &b)
    end

    def grouped_map_reduce(*a, &b)
      a << {:base => a.delete_at(-2)} if a.size >= 2 && a[-2].class != Proc
      super(*a, &b)
    end

    def between(l=nil, r=nil)
      super(Hash[(l ? [['left_bound', l]] : []) + (r ? [['right_bound', r]] : [])])
    end

    def -@; RQL.new.sub(0, self); end

    def [](ind)
      if ind.class == Fixnum
        return nth(ind)
      elsif ind.class == Symbol || ind.class == String
        return getattr(ind)
      elsif ind.class == Range
        if ind.end == 0 && ind.exclude_end?
          raise ArgumentError, "Cannot slice to an excluded end of 0."
        end
        return slice(ind.begin, ind.end - (ind.exclude_end? ? 1 : 0))
      end
      raise ArgumentError, "[] cannot handle #{ind.inspect} of type #{ind.class}."
    end

    def ==(rhs)
      raise ArgumentError,"
      Cannot use inline ==/!= with RQL queries, use .eq() instead if
      you want a query that does equality comparison.

      If you need to see whether two queries are the same, compare
      their protobufs like: `query1.to_pb == query2.to_pb`."
    end

    def do(*a, &b)
      RQL.new.funcall(new_func(&b), *((@body ? [self] : []) + a))
    end
  end
end
