require 'json'
require 'time'

module RethinkDB
  module Shim
    def self.datum_to_native(d, opts)
      raise RqlRuntimeError, "SHENANIGANS" if d.class != Datum
      dt = Datum::DatumType
      case d.type
      when dt::R_NUM then d.r_num == d.r_num.to_i ? d.r_num.to_i : d.r_num
      when dt::R_STR then d.r_str
      when dt::R_BOOL then d.r_bool
      when dt::R_NULL then nil
      when dt::R_ARRAY then d.r_array.map{|d2| datum_to_native(d2, opts)}
      when dt::R_OBJECT then
        obj = Hash[d.r_object.map{|x| [x.key, datum_to_native(x.val, opts)]}]
        if obj["$reql_type$"] == "TIME" && opts[:time_format] != 'raw'
          t = Time.at(obj['epoch_time'])
          tz = obj['timezone']
          (tz && tz != "" && tz != "Z") ? t.getlocal(tz) : t.utc
        else
          obj
        end
      else raise RqlRuntimeError, "Unimplemented."
      end
    end

    def self.response_to_native(r, orig_term, opts)
      rt = Response::ResponseType
      if r.backtrace
        bt = r.backtrace.frames.map {|x|
          x.type == Frame::FrameType::POS ? x.pos : x.opt
        }
      else
        bt = []
      end

      begin
        case r.type
        when rt::SUCCESS_ATOM then datum_to_native(r.response[0], opts)
        when rt::SUCCESS_PARTIAL then r.response.map{|d| datum_to_native(d, opts)}
        when rt::SUCCESS_SEQUENCE then r.response.map{|d| datum_to_native(d, opts)}
        when rt::RUNTIME_ERROR then
          raise RqlRuntimeError, "#{r.response[0].r_str}"
        when rt::COMPILE_ERROR then # TODO: remove?
          raise RqlCompileError, "#{r.response[0].r_str}"
        when rt::CLIENT_ERROR then
          raise RqlDriverError, "#{r.response[0].r_str}"
        else raise RqlRuntimeError, "Unexpected response: #{r.inspect}"
        end
      rescue RqlError => e
        term = orig_term.dup
        term.bt_tag(bt)
        $t = term
        raise e.class, "#{e.message}\nBacktrace:\n#{RPP.pp(term)}"
      end
    end

    def self.native_to_datum_term x
      dt = Datum::DatumType
      d = Datum.new
      case x
      when Fixnum     then d.type = dt::R_NUM;  d.r_num = x
      when Float      then d.type = dt::R_NUM;  d.r_num = x
      when Bignum     then d.type = dt::R_NUM;  d.r_num = x
      when String     then d.type = dt::R_STR;  d.r_str = x
      when Symbol     then d.type = dt::R_STR;  d.r_str = x.to_s
      when TrueClass  then d.type = dt::R_BOOL; d.r_bool = x
      when FalseClass then d.type = dt::R_BOOL; d.r_bool = x
      when NilClass   then d.type = dt::R_NULL
      else raise RqlRuntimeError, "UNREACHABLE"
      end
      t = Term.new
      t.type = Term::TermType::DATUM
      t.datum = d
      return t
    end
  end

  class RQL
    def to_pb; @body; end

    @@datum_types = [Fixnum, Float, Bignum, String, Symbol,
                     TrueClass, FalseClass, NilClass]

    def any_to_pb(x, context)
      return x.to_pb if x.class == RQL
      t = Term.new
      t.type = Term::TermType::JSON
      t.args = [Shim.native_to_datum_term(x.to_json(:max_nesting => 500))]
      return t
    end

    def timezone_from_offset(offset)
      raw_offset = offset.abs
      raw_hours = raw_offset / 3600
      raw_minutes = (raw_offset / 60) - (raw_hours * 60)
      return (offset < 0 ? "-" : "+") + sprintf("%02d:%02d", raw_hours, raw_minutes);
    end

    def fast_expr(x, context, allow_json)
      return x if x.class == RQL
      if @@datum_types.include?(x.class)
        return x if allow_json
        return RQL.new(Shim.native_to_datum_term(x), nil, context)
      end

      case x
      when Array
        args = x.map{|y| fast_expr(y, context, allow_json)}
        return x if allow_json && args.all?{|y| y.class != RQL}
        t = Term.new
        t.type = Term::TermType::MAKE_ARRAY
        t.args = args.map{|y| any_to_pb(y, context)}
        return RQL.new(t, nil, context)
      when Hash
        kvs = x.map{|k,v| [k, fast_expr(v, context, allow_json)]}
        return x if allow_json && kvs.all? {|k,v|
          (k.class == String || k.class == Symbol) && v.class != RQL
        }
        t = Term.new
        t.type = Term::TermType::MAKE_OBJ
        t.optargs = kvs.map{|k,v|
          ap = Term::AssocPair.new;
          if k.class == Symbol || k.class == String
            ap.key = k.to_s
          else
            raise RqlDriverError, "Object keys must be strings or symbols." +
              "  (Got object `#{k.inspect}` of class `#{k.class}`.)"
          end
          ap.val = any_to_pb(v, context)
          ap
        }
        return RQL.new(t, nil, context)
      when Proc
        t = RQL.new(nil, nil, context).new_func(&x).to_pb
        return RQL.new(t, nil, context)
      else raise RqlDriverError, "r.expr can't handle #{x.inspect} of type #{x.class}"
      end
    end

    def check_depth depth
      raise RqlRuntimeError, "Maximum expression depth of 20 exceeded." if depth > 20
    end

    def reql_typify(tree, depth=0)
      check_depth(depth)
      case tree
      when Array
        return tree.map{|x| reql_typify(x, depth+1)}
      when Hash
        return Hash[tree.map{|k,v| [k, reql_typify(v, depth+1)]}]
      when Time
        return {
          '$reql_type$' => 'TIME',
          'epoch_time'  => tree.to_f,
          'timezone'    => timezone_from_offset(tree.utc_offset)
        }
      else
        return tree
      end
    end

    def expr(x, opts={})
      allow_json = opts[:allow_json]
      unbound_if @body
      context = RPP.sanitize_context(caller)
      res = fast_expr(reql_typify(x), context, allow_json)
      return res if res.class == RQL
      return RQL.new(any_to_pb(res, context), nil, context)
    end

    def coerce(other)
      [RQL.new.expr(other), self]
    end
  end
end
