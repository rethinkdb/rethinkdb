$class_types = {
  Query => Query::QueryType, WriteQuery => WriteQuery::WriteQueryType,
  Term => Term::TermType, Builtin => Builtin::BuiltinType }
def enum_type(_class, _type); _class.values[_type.to_s.upcase.to_sym]; end
def message_field(_class, _type); _class.fields.select{|x,y| y.name == _type}[0]; end
def message_set(message, key, val); message.send((key.to_s+'=').to_sym, val); end

def handle_special_cases(message, query_type, query_args)
  case query_type
  when :compare then
    message.comparison = enum_type(Builtin::Comparison, query_args)
    throw :unknown_comparator_error if not message.comparison
    return true
  else return false
  end
end

def maybe_rewrite_query_type(query_type)
  { :getattr => :attr, :implicit_getattr => :attr,
    :hasatter => :attr, :implicit_hasattr => :attr,
    :pickattrs => :attrs, :implicit_pickattrs => :attrs,
    :string => :valuestring, :json => :jsonstring, :bool => :valuebool,
    :if => :if_, :getbykey => :get_by_key
  }[query_type] || query_type
end

def is_trampoline(query_type)
  ([:table, :map, :filter, :concatmap]).include?(query_type)
end

def parse(message_class, args, repeating=false)
  #PP.pp(["A", message_class, args, repeating])
  if repeating; return args.map {|arg| parse(message_class, arg)}; end
  args = args[0] if args.kind_of? Array and args[0].kind_of? Hash
  throw "Cannot construct #{message_class} from #{args}." if args == []
  if message_class.kind_of? Symbol
    args = [args] if args.class != Array
    throw "Cannot construct #{message_class} from #{args}." if args.length != 1
    return args[0]
  end

  message = message_class.new
  if (message_type_class = $class_types[message_class])
    args = $reql.expr(args).sexp if args.class() != Array
    query_type = args[0]
    message.type = enum_type(message_type_class, query_type)
    return message if args.length == 1

    query_args = args[1..args.length]
    query_args = [query_args] if is_trampoline query_type
    return message if handle_special_cases(message, query_type, query_args)

    query_type = maybe_rewrite_query_type(query_type)
    field_metadata = message_class.fields.select{|x,y| y.name == query_type}[0]
    throw "No field '#{query_type}' in '#{message_class}'." if not field_metadata
    field = field_metadata[1]
    message_set(message, query_type, parse(field.type, query_args,field.rule==:repeated))
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
