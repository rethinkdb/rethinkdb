require 'rubygems'
require 'query_language.pb.rb'
require 'socket'

$class_types = {
  Query => Query::QueryType,
  WriteQuery => WriteQuery::WriteQueryType,
  Term => Term::TermType,
  Builtin => Builtin::BuiltinType
}

def get_enum_type(type_class, query_type)
  type_class.values[query_type.to_s.upcase.to_sym]
end
def message_set(message, key, val)
  message.send((key.to_s+'=').to_sym, val)
end

def handle_special_cases(message, query_type, query_args)
  if query_type == :compare
    comp = get_enum_type(Builtin::Comparison, query_args)
    throw :error if not comp
    message.comparison = comp
    return message
  else
    return nil
  end
end

$query_rewrites = {
  :getattr => :attr,
  :implicit_getattr => :attr,
  :hasatter => :attr,
  :implicit_hasattr => :attr,
  :pickattrs => :attrs,
  :implicit_pickattrs => :attrs
}

def maybe_rewrite_query_type(query_type)
  new_query_type = $query_rewrites[query_type]
  if new_query_type
    return new_query_type
  else
    return query_type
  end
end

def parse(message_class, args, repeating=false)
  return args.map {|arg| parse(message_class, arg)} if repeating
  throw :error if args == []

  if message_class.kind_of? Symbol
    throw :error if args.length != 1
    return args[0]
  end

  message = message_class.new
  message_type_class = $class_types[message_class]
  if message_type_class
    query_type = args[0]
    message.type = get_enum_type(message_type_class, query_type)
    return message if args.length == 1
    query_args = args[1..args.length]

    alt_message = handle_special_cases(message, query_type, query_args)
    return alt_message if alt_message
    query_type = maybe_rewrite_query_type(query_type)

    field_metadata = message_class.fields.select{|x,y| y.name == query_type}[0]
    throw :error if not field_metadata
    field_type = field_metadata[1].type
    message_set(message, query_type, parse(field_type, query_args))
  else
    if args.kind_of? Hash
      message.fields.each { |field|
        field = field[1]
        arg = args[field.name]
        if arg
          message_set(message, field.name, parse(field.type, arg, field.rule==:repeated))
        end
      }
    else
      message.fields.zip(args).each { |params|
        field = params[0][1]
        arg = params[1]
        if arg
          message_set(message, field.name, parse(field.type, arg, field.rule==:repeated))
        end
      }
    end
  end
  return message
end

parse(Term,
      [:call,
       [:filter,
        {:arg => ["row"],
         :body => [:call,
                   [:all],
                   [[:call,
                     [:implicit_getattr, "a"],
                     [[:number, 0]]]]]}],
        [[:table, {:table_name => ["Welcome"]}]]])
