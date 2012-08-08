module RethinkDB
  module RQL_Protob_Mixin
    include P_Mixin
    @@token = 0
    def handle_special_cases(message, query_type, query_args)
      case query_type
      when :compare then
        message.comparison = enum_type(Builtin::Comparison, query_args)
        throw :unknown_comparator_error if not message.comparison
        return true
      else return false
      end
    end

    #TODO: complain if leftover args
    def comp(message_class, args, repeating=false)
      #PP.pp(["A", message_class, args, repeating])
      if repeating; return args.map {|arg| comp(message_class, arg)}; end
      args = args[0] if args.class == Array and args[0].class == Hash
      if args == []; throw "Cannot construct #{message_class} from #{args}."; end
      if message_class.kind_of? Symbol
        args = [args] if args.class != Array
        throw "Cannot construct #{message_class} from #{args}." if args.length != 1
        args[0] = args[0].to_s if args[0].class == Symbol
        return args[0]
      end

      message = message_class.new
      if (message_type_class = C.class_types[message_class])
        args = RQL.expr(args).sexp if args.class() != Array
        query_type = args[0]
        message.type = enum_type(message_type_class, query_type)
        throw "No type '#{query_type}' for '#{message_class}'." if not message.type
        return message if args.length == 1

        query_args = args[1..-1]
        query_args = [query_args] if C.trampolines.include? query_type
        return message if handle_special_cases(message, query_type, query_args)

        query_type = C.query_rewrites[query_type] || query_type
        field_metadata = message_class.fields.select{|x,y| y.name == query_type}[0]
        throw "No field '#{query_type}' in '#{message_class}'." if not field_metadata
        field = field_metadata[1]
        message_set(message, query_type,
                    comp(field.type, query_args,field.rule==:repeated))
      elsif args.class == Hash
        #TODO: How to map over hash?
        args_tmp = {}
        args = args.each{|k,v| args_tmp[k] = (v == nil ? RQL.expr(v) : v)}
        args = args_tmp
        message.fields.each {|_field|; field = _field[1]; arg = args[field.name]
          message_set(message, field.name,
                      comp(field.type, arg, field.rule==:repeated)) if arg != nil
        }
      elsif args.class == Array
        args = args.map{|x| x == nil ? RQL.expr(x) : x}
        message.fields.zip(args).each {|_params|;field = _params[0][1];arg = _params[1]
          message_set(message, field.name,
                      comp(field.type, arg, field.rule==:repeated)) if arg != nil
        }
      else message = comp(message_class, [args], repeating)
        #else throw "Don't know how to handle args '#{args}' of type '#{args.class}'."
      end
      return message
    end

    #TODO: when nothing is called, just direct in SEXP?
    def query(sexp)
      if sexp.class == Array and enum_type(WriteQuery::WriteQueryType, sexp[0])
      then q = comp(Query, [:write, *sexp])
      else q = comp(Query, [:read, sexp])
      end
      q.token = (@@token += 1)
      return q
    end
  end
  module RQL_Protob; extend RQL_Protob_Mixin; end
end
