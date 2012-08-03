require 'rubygems'
require 'query_language.pb.rb'
require 'socket'
require 'pp'

module RethinkDB
  module C #Constants
    def self.method_aliases
      { :equals => :eq, :neq => :neq,
        :neq => :ne, :< => :lt, :<= => :le, :> => :gt, :>= => :ge,
        :+ => :add, :- => :subtract, :* => :multiply, :/ => :divide, :% => :modulo } end
    def self.class_types
      { Query => Query::QueryType, WriteQuery => WriteQuery::WriteQueryType,
        Term => Term::TermType, Builtin => Builtin::BuiltinType } end
    def self.query_rewrites
      { :getattr => :attr, :implicit_getattr => :attr,
        :hasatter => :attr, :implicit_hasattr => :attr,
        :pickattrs => :attrs, :implicit_pickattrs => :attrs,
        :string => :valuestring, :json => :jsonstring, :bool => :valuebool,
        :if => :if_, :getbykey => :get_by_key } end
    def self.trampolines
      [:table, :map, :filter, :concatmap] end
  end

  module RQL_Protob_Mixin
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
        args = $reql.expr(args).sexp if args.class() != Array
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
      else
        if args.kind_of? Hash
          message.fields.each { |field|
            field = field[1]
            arg = args[field.name]
            if arg
              message_set(message, field.name,
                          comp(field.type, arg, field.rule==:repeated))
            end
          }
        else
          message.fields.zip(args).each { |params|
            field = params[0][1]
            arg = params[1]
            if arg
              message_set(message, field.name,
                          comp(field.type, arg, field.rule==:repeated))
            end
          }
        end
      end
      return message
    end
  end
  module RQL_Protob; extend RQL_Protob_Mixin; end

  class S #Sexp
    def clean_lst lst
      case lst.class.hash
      when Array.hash then lst.map{|z| clean_lst(z)}
      when S.hash     then lst.sexp
      else lst
      end
    end
    def initialize(init_body); @body = init_body; end
    def sexp; clean_lst @body; end
    def protob; RQL_Protob.comp(Term, sexp); end

    def [](ind); S.new [:call, [:getattr, ind.to_s], [@body]]; end
    def getbykey(attr, key)
      throw "getbykey must be called on a table" if @body[0] != :table
      S.new [:getbykey, @body[1..3], attr, RQL.expr(key)]
    end

    def method_missing(meth, *args, &block)
      meth = C.method_aliases[meth] || meth
      if enum_type(Builtin::Comparison, meth)
        S.new [:call, [:compare, meth], [@body, *args]]
      elsif enum_type(Builtin::BuiltinType, meth)
        if message_field(Builtin, meth) then S.new [:call, [meth, *args], [@body]]
        else S.new [:call, [meth], [@body, *args]]
        end
      else super(meth, *args, &block)
      end
    end
  end

  module RQL_Mixin
    def attr a; S.new [:call, [:implicit_getattr, a], []]; end
    def expr x
      case x.class().hash
      when S.hash          then x
      when String.hash     then S.new [:string, x]
      when Fixnum.hash     then S.new [:number, x]
      when TrueClass.hash  then S.new [:bool, x]
      when FalseClass.hash then S.new [:bool, x]
      when NilClass.hash   then S.new [:json_null]
      when Array.hash      then S.new [:array, *x.map{|y| expr(y)}]
      when Hash.hash       then S.new [:object, *x.map{|var,term| [var, expr(term)]}]
      when Symbol.hash     then
        str = x.to_s
        S.new str[0] == '$'[0] ? var(str[1..str.length]) : attr(str)
      else raise TypeError, "term.expr can't handle type `#{x.class()}`"
      end
    end
    def method_missing(meth, *args, &block)
      meth = C.method_aliases[meth] || meth
      if    enum_type(Builtin::BuiltinType, meth) then S.new [:call,[meth],args]
      elsif enum_type(Builtin::Comparison, meth)  then S.new [:call,[:compare,meth],args]
      elsif enum_type(Term::TermType, meth)       then S.new [meth,*args]
      else super(meth, *args, &block)
      end
    end
  end
  module RQL; extend RQL_Mixin; end
end
