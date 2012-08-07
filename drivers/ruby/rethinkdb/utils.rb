module RethinkDB
  module C_Mixin #Constants
    def method_aliases
      { :attr => :getattr, :attrs => :pickattrs, :attr? => :hasattr,
        :equals => :eq, :neq => :neq,
        :neq => :ne, :< => :lt, :<= => :le, :> => :gt, :>= => :ge,
        :sub => :subtract, :mul => :multiply, :div => :divide, :mod => :modulo,
        :+ => :add, :- => :subtract, :* => :multiply, :/ => :divide, :% => :modulo,
        :and => :all, :& => :all, :or => :any, :| => :any } end
    def class_types
      { Query => Query::QueryType, WriteQuery => WriteQuery::WriteQueryType,
        Term => Term::TermType, Builtin => Builtin::BuiltinType } end
    def query_rewrites
      { :getattr => :attr, :implicit_getattr => :attr,
        :hasattr => :attr, :implicit_hasattr => :attr,
        :pickattrs => :attrs, :implicit_pickattrs => :attrs,
        :string => :valuestring, :json => :jsonstring, :bool => :valuebool,
        :if => :if_, :getbykey => :get_by_key, :groupedmapreduce => :grouped_map_reduce,
        :insertstream => :insert_stream, :foreach => :for_each, :orderby => :order_by,
        :pointupdate => :point_update, :pointdelete => :point_delete,
        :pointmutate => :point_mutate,
        :read => :read_query, :write => :write_query } end
    def trampolines; [:table, :map, :concatmap, :filter] end
    def repeats; [:insert, :foreach]; end
    def table_directs
      [:insert, :insertstream, :pointupdate, :pointdelete, :pointmutate] end
  end
  module C; extend C_Mixin; end

  module P_Mixin #Protobuf Utils
    def enum_type(_class, _type); _class.values[_type.to_s.upcase.to_sym]; end
    def message_field(_class, _type); _class.fields.select{|x,y| y.name == _type}[0]; end
    def message_set(message, key, val); message.send((key.to_s+'=').to_sym, val); end
  end
  module P; extend P_Mixin; end
end
