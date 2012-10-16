module RethinkDB
  module C_Mixin #Constants
    # We often have shorter names for things, or inline variants, specified here.
    def method_aliases
      { :attr => :getattr, :attrs => :pickattrs, :attr? => :hasattr,
        :get => :getattr, :pick => :pickattrs, :has => :hasattr,
        :equals => :eq, :neq => :neq,
        :neq => :ne, :< => :lt, :<= => :le, :> => :gt, :>= => :ge,
        :sub => :subtract, :mul => :multiply, :div => :divide, :mod => :modulo,
        :+ => :add, :- => :subtract, :* => :multiply, :/ => :divide, :% => :modulo,
        :and => :all, :& => :all, :or => :any, :| => :any, :js => :javascript,
        :to_stream => :arraytostream, :to_array => :streamtoarray} end

    # Allows us to identify protobuf classes which are actually variant types,
    # and to get their corresponding enums.
    def class_types
      { Query => Query::QueryType, WriteQuery => WriteQuery::WriteQueryType,
        Term => Term::TermType, Builtin => Builtin::BuiltinType,
        MetaQuery => MetaQuery::MetaQueryType} end

    # The protobuf spec often has a field with a slightly different name from
    # the enum constant, which we list here.
    def query_rewrites
      { :getattr => :attr,
        :hasattr => :attr,
        :without => :attrs,
        :pickattrs => :attrs,
        :string => :valuestring, :json => :jsonstring, :bool => :valuebool,
        :if => :if_, :getbykey => :get_by_key, :groupedmapreduce => :grouped_map_reduce,
        :insertstream => :insert_stream, :foreach => :for_each, :orderby => :order_by,
        :pointupdate => :point_update, :pointdelete => :point_delete,
        :pointmutate => :point_mutate, :concatmap => :concat_map,
        :read => :read_query, :write => :write_query, :meta => :meta_query,
        :create_db => :db_name, :drop_db => :db_name, :list_tables => :db_name
      } end

    # These classes go through a useless intermediate type.
    def trampolines; [:table, :map, :concatmap, :filter] end

    # These classes expect repeating arguments.
    def repeats; [:insert, :foreach]; end

    # These classes reference a tableref directly, rather than a term.
    def table_directs
      [:insert, :insertstream, :pointupdate, :pointdelete, :pointmutate,
       :create, :drop] end

    def nonatomic_variants
      [:update_nonatomic, :mutate_nonatomic,
       :pointupdate_nonatomic, :pointmutate_nonatomic] end
  end
  module C; extend C_Mixin; end

  module P_Mixin #Protobuf Utils
    def enum_type(_class, _type); _class.values[_type.to_s.upcase.to_sym]; end
    def message_field(_class, _type); _class.fields.select{|x,y|
        y.instance_eval{@name} == _type
        #TODO: why did the following stop working?  (Or, why did it ever work?)
        #y.name == _type
      }[0]; end
    def message_set(message, key, val); message.send((key.to_s+'=').to_sym, val); end
  end
  module P; extend P_Mixin; end

  module S_Mixin #S-expression Utils
    @@gensym_counter = 1000
    def gensym; '_var_'+(@@gensym_counter += 1).to_s; end
    def with_var
      sym = gensym
      yield sym, var(sym)
    end
    def var(varname)
      B.alt_inspect(Var_Expression.new [:var, varname]) { |expr| expr.body[1] }
    end
    def r x; x.equal?(skip) ? x : RQL.expr(x); end
    def skip; :skip_2222ebd4_2c16_485e_8c27_bbe43674a852; end
    def conn_outdated; :conn_outdated_2222ebd4_2c16_485e_8c27_bbe43674a852; end

    def arg_or_block(*args, &block)
      if args.length == 1 && block
        raise ArgumentError,"Cannot provide both an argument *and* a block."
      end
      return lambda { args[0] } if (args.length == 1)
      return block if block
      raise ArgumentError,"Must provide either an argument or a block."
    end

    def clean_lst lst # :nodoc:
      return lst.sexp if lst.kind_of? RQL_Query
      return lst.class == Array ? lst.map{|z| clean_lst(z)} : lst
    end

    def replace(sexp, map)
      return sexp.map{|x| replace x,map} if sexp.class == Array
      return (map.include? sexp) ? map[sexp] : sexp
    end

    def checkdict s
      return s.to_s if s.class == String || s.class == Symbol
      raise RuntimeError, "Invalid JSON dict key: #{s.inspect}"
    end
  end
  module S; extend S_Mixin; end

  module B_Mixin # Backtrace utils
    attr_accessor :highlight, :line

    def sanitize_context(context)
      if __FILE__ =~ /^(.*\/)[^\/]+.rb$/
        prefix = $1;
        context.reject{|x| x =~ /^#{prefix}/}
      else
        context
      end
    end

    def format_highlights(str)
      str.chars.map{|x| x == "\000" ? "^" : " "}.join.rstrip
    end

    def force_raise(query)
      obj = query.run
      if obj.class == Query_Results
        obj = obj.to_a
      elsif (err = obj['first_error'])
        parts = err.split("\nBacktrace:\n")
        errline = parts[0] || ""
        bt = (parts[1] && parts[1].split("\n")) || []
        raise RuntimeError,errline+"\n"+query.print_backtrace(bt)
      end
      obj
    end

    def set(sym,val)
      @map = {} if not @map
      res, @map[sym] = @map[sym], val
      return res
    end
    def consume sym
      @map = {} if not @map
      res, @map[sym] = @map[sym], nil
      return res.nil? ? 0 : res
    end
    def expand arg
      case arg
      when 'attr'           then []
      when 'attrname'       then []
      when 'base'           then [1+consume(:reduce)]
      when 'body'           then [4+consume(:reduce)]
      when 'datacenter'     then []
      when 'db_name'        then []
      when 'expr'           then [2]
      when 'false'          then [3]
      when 'group_mapping'  then [1, 1, 1]
      when 'key'            then [3]
      when 'keyname'        then []
      when 'lowerbound'     then [1, 2]
      when 'mapping'        then [1, 2]
      when 'modify_map'     then [2, 1]
      when 'order_by'       then []
      when 'point_map'      then [4, 1]
      when 'predicate'      then [1, 2]
      when 'reduce'         then [1]
      when 'reduction'      then set(:reduce, -1); [1, 3]
      when 'stream'         then [1]
      when 'table_name'     then []
      when 'table_ref'      then []
      when 'test'           then [1]
      when 'true'           then [2]
      when 'upperbound'     then [1, 3]
      when 'value_mapping'  then [1, 2, 1]
      when 'view'           then [1]
      when /arg:([0-9]+)/   then [2, $1.to_i]
      when /attrs:([0-9]+)/ then []
      when /elem:([0-9]+)/  then [$1.to_i+1]
      when /bind:(.+)$/     then [1, $1]
      when /key:(.+)$/      then [1..-1, $1]
      when /query:([0-9]+)/ then [3, $1.to_i]
      when /term:([0-9]+)/  then [2, $1.to_i]
      else  [:error]
      end
    end

    def recur(arr, lst, val)
      if lst[0].class == String
        entry = arr[0..-1].find{|x| x[0].to_s == lst[0]}
        raise RuntimeError if not entry
        mark(entry[1], lst[1..-1], val)
      elsif lst[0].class == Fixnum || lst[0].class == Range
        mark(arr[lst[0]], lst[1..-1], val) if lst != []
      else
        raise RuntimeError
      end
    end
    def mark(query, lst, val)
      #PP.pp [lst, query.class, query]
      if query.class == Array
        recur(query, lst, val) if lst != []
      elsif query.kind_of? RQL_Query
        if lst == []
          @line = sanitize_context(query.context)[0]
          val,query.marked = query.marked, val
          val
        else
          recur(query.body, lst, val)
        end
      else raise RuntimeError
      end
    end

    def with_marked_error(query, bt)
      @map = {}
      bt = bt.map{|x| expand x}.flatten
      raise RuntimeError if bt.any?{|x| x == :error}
      old = mark(query, bt, :error)
      res = yield
      mark(query, bt, old)
      return res
    end

    def with_highlight
      @highlight = true
      str = yield
      @highlight = false
      str.chars.map{|x| x == "\000" ? "^" : " "}.join.rstrip
    end

    def alt_inspect(query, &block)
      class << query
        attr_accessor :str_block
        def inspect; real_inspect({:str => @str_block.call(self)}); end
      end
      query.str_block = block
      query
    end
  end
  module B; extend B_Mixin; end
end
