module RethinkDB
    module RQL_Protob_Mixin
    include P_Mixin
    def handle_special_cases(message, query_type, query_args)
      case query_type
      when :compare then
        message.comparison = enum_type(Builtin::Comparison, query_args)
        throw :unknown_comparator_error if not message.comparison
        return true
      else return false
      end
    end

    def comp(message_class, args, repeating=false)
      #PP.pp(["A", message_class, args, repeating])
      if repeating; return args.map {|arg| comp(message_class, arg)}; end
      args = args[0] if args.kind_of? Array and args[0].kind_of? Hash
      throw "Cannot construct #{message_class} from #{args}." if args == []
      if message_class.kind_of? Symbol
        args = [args] if args.class != Array
        throw "Cannot construct #{message_class} from #{args}." if args.length != 1
        return args[0]
      end

      message = message_class.new
      if (message_type_class = C.class_types[message_class])
        args = RQL.expr(args).sexp if args.class() != Array
        query_type = args[0]
        message.type = enum_type(message_type_class, query_type)
        return message if args.length == 1

        query_args = args[1..args.length]
        query_args = [query_args] if C.trampolines.include? query_type
        return message if handle_special_cases(message, query_type, query_args)

        query_type = C.query_rewrites[query_type] || query_type
        field_metadata = message_class.fields.select{|x,y| y.name == query_type}[0]
        throw "No field '#{query_type}' in '#{message_class}'." if not field_metadata
        field = field_metadata[1]
        message_set(message, query_type,
                    comp(field.type, query_args,field.rule==:repeated))
      elsif args.class == Hash
        message.fields.each {|_field|; field = _field[1]; arg = args[field.name]
          message_set(message, field.name,
                      comp(field.type, arg, field.rule==:repeated)) if arg
        }
      elsif args.class == Array
        message.fields.zip(args).each {|_params|; field = _params[0][1]; arg = _params[1]
          message_set(message, field.name,
                      comp(field.type, arg, field.rule==:repeated)) if arg
        }
      else
        throw "Don't know how to handle arg '#{arg}' of type '#{arg.class}'."
      end
      return message
    end
  end
  module RQL_Protob; extend RQL_Protob_Mixin; end
end
