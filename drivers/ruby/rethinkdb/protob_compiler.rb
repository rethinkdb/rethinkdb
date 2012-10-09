module RethinkDB
  module RQL_Protob_Mixin
    include P_Mixin
    @@token = 0 # We need a new token every time we compile a query
    @@backtrace = []

    # Right now the only special case is :compare, but in general we might have
    # terms that don't follow the conventions and we need a place to store that.
    def handle_special_cases(message, query_type, query_args)
      #PP.pp ["special_case", message, query_type, query_args]
      case query_type
      when :compare then
        message.comparison = enum_type(Builtin::Comparison, query_args)
        throw :unknown_comparator_error if not message.comparison
        return true
      else return false
      end
    end

################################################################################
    # I apologize for the code after this.  I was originally doing something
    # clever to generate the S-expressions, so they don't always match the
    # protobufs perfectly, and the complexity necessary to handle that got
    # pushed down into protobuf compilation.  Later the cleverness went away,
    # but I left the code below intact rather than refactoring for two reasons:
    #   1) It's fragile and took about a day to get right, so refactoring it
    #      probably isn't free.
    #   2) We are probably going to have to change protobuf implementations for
    #      speed reasons anyway, at which point this will have to be rewritten,
    #      and rewriting it twice doesn't make sense
################################################################################

    # Compile an RQL query into a protobuf.
    def comp(message_class, args, repeating=false)
      #PP.pp(["A", message_class, args, repeating]) #DEBUG

      # Handle special cases of arguments:
      #   * When we're compiling a repeating term, in which case we want to
      #     compile the elements of [args] rather than [args] itself.
      #   * Objects, which correspond to optional argument specifications.
      #   * Empty arguments
      return args.map {|arg| comp(message_class, arg)} if repeating
      args = args[0] if args.class == Array and args[0].class == Hash
      raise TypeError,"Can't build #{message_class} from #{args.inspect}." if args == []

      # Handle terminal parts of the protobuf, where we have to actually pack values.
      if message_class.kind_of? Symbol
        # It's easier for the user if we allow both atoms and 1-element lists
        args = [args] if args.class != Array
        if args.length != 1
        then raise TypeError,"Can't build #{message_class} from #{args.inspect}." end
        # Coercing symbols into strings makes our attribute notation more consistent
        args[0] = args[0].to_s if args[0].class == Symbol
        return args[0]
      end

      # Handle nonterminal parts of the protobuf, where we have to descend
      message = message_class.new
      if (message_type_class = C.class_types[message_class])
        query_type = args[0] # Our first argument is the type of our variant.
        message.type = enum_type(message_type_class, query_type)
        raise TypeError,"No type '#{query_type}' for '#{message_class}'."if !message.type

        if args.length == 1 # Our variant takes no arguments of its own.
          return message
        else # Compile our variant's arguments.
          query_args = args[1..-1]
          query_args = [query_args] if C.trampolines.include? query_type
          return message if handle_special_cases(message, query_type, query_args)

          # The general, non-special case.
          query_type = C.query_rewrites[query_type] || query_type
          field_metadata = message_class.fields.select{|x,y| y.name == query_type}[0]
          if not field_metadata
          then raise SyntaxError,"No field '#{query_type}' in '#{message_class}'." end
          field = field_metadata[1]
          message_set(message, query_type,
                      comp(field.type, query_args,field.rule==:repeated))
        end
      # Handle cases where we aren't constructing a toplevel variant type.
      elsif args.class == Array
        # Handle the case where we're constructinga the fields in order.
        args = args.map{|x| x == nil ? RQL.expr(x) : x}
        fields = message.fields.sort_by {|kv| kv[0]}
        fields.zip(args).each {|_params|;field = _params[0][1];arg = _params[1]
          if arg == S.skip
            if field.rule != :optional
              raise RuntimeError, "Cannot skip non-optional rule."
            end
          else
            message_set(message, field.name,
                        comp(field.type, arg, field.rule==:repeated)) if arg != nil
          end
        }
      else
        # Handle the case where the user provided neither an Array nor a Hash,
        # in which case they probably provided an Atom that we should treat as a
        # single argument (allowing this makes the interface cleaner).
        message = comp(message_class, [args], repeating)
      end
      return message
    rescue => e # Add our own, semantic backtrace to exceptions
      new_msg = repeating ? e.message :
        e.message + "\n...when compiling #{message_class} with:\n#{args.pretty_inspect}"
      raise e.class, new_msg
    end

    def handle_special_query_cases(sexp)
      if C.nonatomic_variants.include?(sexp[0])
        sexp[0] = sexp[0].to_s.split('_')[0].to_sym
        res = query(sexp)
        res.write_query.atomic = false
        return res
      end
      return nil
    end

    # Construct a protobuf query from an RQL query by inferring the query type.
    def query(sexp)
      raise TypeError, "Cannot build query from #{sexp.inspect}" if sexp.class != Array
      if (m = handle_special_query_cases(sexp))
      then q = m
      elsif enum_type(WriteQuery::WriteQueryType, sexp[0])
      then q = comp(Query, [:write, *sexp])
      elsif enum_type(MetaQuery::MetaQueryType, sexp[0])
      then q = comp(Query, [:meta, *sexp])
      else q = comp(Query, [:read, sexp])
      end
      q.token = (@@token += 1)
      return q
    end
  end
  module RQL_Protob; extend RQL_Protob_Mixin; end
end
