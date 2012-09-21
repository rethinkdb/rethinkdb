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
      { :getattr => :attr, :implicit_getattr => :attr,
        :hasattr => :attr, :implicit_hasattr => :attr,
        :without => :attrs, :implicit_without => :attrs,
        :pickattrs => :attrs, :implicit_pickattrs => :attrs,
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
    @@gensym_counter = 0
    def gensym; 'gensym_'+(@@gensym_counter += 1).to_s; end
    def with_var; sym = gensym; yield sym, var(sym); end
    def var(varname); Var_Expression.new [:var, varname]; end
    def r x; RQL.expr(x); end
    def skip; :skip_2222ebd4_2c16_485e_8c27_bbe43674a852; end

    def arg_or_block(*args, &block)
      if args.length == 1 && block
        raise SyntaxError,"Cannot provide both an argument *and* a block."
      end
      return lambda { args[0] } if (args.length == 1)
      return block if block
      raise SyntaxError,"Must provide either an argument or a block."
    end

    def clean_lst lst # :nodoc:
      return lst.sexp if lst.kind_of? RQL_Query
      return lst.class == Array ? lst.map{|z| clean_lst(z)} : lst
    end

    def replace(sexp, old, new)
      return sexp.map{|x| replace x,old,new} if sexp.class == Array
      return (sexp == old) ? new : sexp
    end
  end
  module S; extend S_Mixin; end
end
