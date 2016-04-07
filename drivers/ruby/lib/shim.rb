module RethinkDB
  require 'json'
  require 'time'
  require 'base64'

  # Use a dummy object for binary data so we don't lose track of the data type
  class Binary < String
    def inspect
      out = ''
      if length > 0
        arr = [ ]
        byteslice(0, 6).each_byte{|c| arr << sprintf('%02x', c.ord)}
        out = sprintf(', \'%s%s\'', arr.join(' '), length > 6 ? '...' : '')
      end
      return sprintf('<binary, %d byte%s%s>',
                     length, length == 1 ? '' : 's', out)
    end
  end

  module Shim
    def self.recursive_munge(x, parse_time, parse_group, parse_binary)
      case x
      when Hash
        if parse_time && x['$reql_type$'] == 'TIME'
          t = Time.at(x['epoch_time'])
          tz = x['timezone']
          return (tz && tz != "" && tz != "Z") ? t.getlocal(tz) : t.utc
        elsif parse_group && x['$reql_type$'] == 'GROUPED_DATA'
          return Hash[recursive_munge(x['data'], parse_time, parse_group, parse_binary)]
        elsif parse_binary && x['$reql_type$'] == 'BINARY'
          return Binary.new(Base64.decode64(x['data'])).force_encoding('BINARY')
        else
          x.each {|k, v|
            v2 = recursive_munge(v, parse_time, parse_group, parse_binary)
            x[k] = v2 if v.object_id != v2.object_id
          }
        end
      when Array
        x.each_with_index {|v, i|
          v2 = recursive_munge(v, parse_time, parse_group, parse_binary)
          x[i] = v2 if v.object_id != v2.object_id
        }
      end
      return x
    end

    def self.load_json(target, opts=nil)
      recursive_munge(JSON.parse(target, max_nesting: false),
                      opts && opts[:time_format] != 'raw',
                      opts && opts[:group_format] != 'raw',
                      opts && opts[:binary_format] != 'raw')
    end

    def self.dump_json(x)
      JSON.generate(x, max_nesting: false)
    end

    def self.response_to_native(r, orig_term, opts)
      rt = Response::ResponseType
      re = Response::ErrorType
      begin
        case r['t']
        when rt::SUCCESS_ATOM         then r['r'][0]
        when rt::SUCCESS_PARTIAL      then r['r']
        when rt::SUCCESS_SEQUENCE     then r['r']
        when rt::RUNTIME_ERROR
          case r['e']
            when re::INTERNAL         then raise ReqlInternalError,         r['r'][0]
            when re::RESOURCE_LIMIT   then raise ReqlResourceLimitError,    r['r'][0]
            when re::QUERY_LOGIC      then raise ReqlQueryLogicError,       r['r'][0]
            when re::NON_EXISTENCE    then raise ReqlNonExistenceError,     r['r'][0]
            when re::OP_FAILED        then raise ReqlOpFailedError,         r['r'][0]
            when re::OP_INDETERMINATE then raise ReqlOpIndeterminateError,  r['r'][0]
            when re::USER             then raise ReqlUserError,             r['r'][0]
            when re::PERMISSION_ERROR then raise ReqlPermissionError,       r['r'][0]
            else                           raise ReqlRuntimeError,          r['r'][0]
          end
        when rt::COMPILE_ERROR        then raise ReqlServerCompileError,    r['r'][0]
        when rt::CLIENT_ERROR         then raise ReqlDriverError,           r['r'][0]
        else raise ReqlRuntimeError, "Unexpected response: #{r.inspect}"
        end
      rescue ReqlError => e
        raise e.class, "#{e.message}\nBacktrace:\n#{RPP.pp(orig_term, r['b'])}"
      end
    end
  end

  class RQL
    def to_json(*a, &b)
      @body.to_json(*a, &b)
    end

    def as_json(*a, &b)
      @body.as_json(*a, &b)
    end

    def to_pb; @body; end

    def binary(*a)
        args = ((@body != RQL) ? [self] : []) + a
        RQL.new([Term::TermType::BINARY, args.map {|x|
                    case x
                    when RQL then x.to_pb
                    else { '$reql_type$' => 'BINARY', 'data' => Base64.strict_encode64(x) }
                    end
                 }])
    end

    def self.safe_to_s(x)
      case x
      when String then x
      when Symbol then x.to_s
      else raise ReqlDriverCompileError, 'Object keys must be strings or symbols.  '+
          "(Got object `#{x.inspect}` of class `#{x.class}`.)"
      end
    end

    def self.fast_expr(x, max_depth)
      if max_depth == 0
        raise ReqlDriverCompileError, "Maximum expression depth exceeded " +
          "(you can override this with `r.expr(X, MAX_DEPTH)`)."
      end
      case x
      when RQL then x
      when Array then RQL.new([Term::TermType::MAKE_ARRAY,
                               x.map{|y| fast_expr(y, max_depth-1)}])
      when Hash then RQL.new(Hash[x.map{|k,v| [safe_to_s(k),
                                               fast_expr(v, max_depth-1)]}])
      when Proc then RQL.new.new_func(&x)
      when Binary then RQL.new.binary(x)
      when String then RQL.new(x)
      when Symbol then RQL.new(x)
      when Numeric then RQL.new(x)
      when FalseClass then RQL.new(x)
      when TrueClass then RQL.new(x)
      when NilClass then RQL.new(x)
      when Time then
        epoch_time = x.to_f
        offset = x.utc_offset
        raw_offset = offset.abs
        raw_hours = raw_offset / 3600
        raw_minutes = (raw_offset / 60) - (raw_hours * 60)
        tz = (offset < 0 ? "-" : "+") + sprintf("%02d:%02d", raw_hours, raw_minutes);
        RQL.new({ '$reql_type$' => 'TIME',
                  'epoch_time'  => epoch_time,
                  'timezone'    => tz })
      else raise ReqlDriverCompileError, "r.expr can't handle #{x.inspect} of class #{x.class}."
      end
    end

    def expr(x, max_depth=20)
      if not max_depth.is_a? Numeric
        raise ReqlDriverCompileError, "Second argument to `r.expr` must be a number."
      end
      unbound_if(@body != RQL)
      RQL.fast_expr(x, max_depth)
    end
    def coerce(other)
      [RQL.new.expr(other), self]
    end
  end
end
