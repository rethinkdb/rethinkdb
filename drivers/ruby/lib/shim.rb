module RethinkDB
  module Shim
    def self.datum_to_native d
      raise RqlRuntimeError, "SHENANIGANS" if d.class != Datum
      dt = Datum::DatumType
      case d.type
      when dt::R_NUM then d.r_num == d.r_num.to_i ? d.r_num.to_i : d.r_num
      when dt::R_STR then d.r_str
      when dt::R_BOOL then d.r_bool
      when dt::R_NULL then nil
      when dt::R_ARRAY then d.r_array.map{|d2| datum_to_native d2}
      when dt::R_OBJECT then Hash[d.r_object.map{|x| [x.key, datum_to_native(x.val)]}]
      else raise RqlRuntimeError, "Unimplemented."
      end
    end

    def self.response_to_native(r, orig_term)
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
        when rt::SUCCESS_ATOM then datum_to_native(r.response[0])
        when rt::SUCCESS_PARTIAL then r.response.map{|d| datum_to_native(d)}
        when rt::SUCCESS_SEQUENCE then r.response.map{|d| datum_to_native(d)}
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

    def expr(x)
      unbound_if @body
      return x if x.class == RQL
      datum_types = [Fixnum, Float, Bignum, String, Symbol,
                     TrueClass, FalseClass, NilClass]

      if datum_types.include? x.class
        return RQL.new(Shim.native_to_datum_term(x))
      end

      t = Term.new
      case x
      when Array
        t.type = Term::TermType::MAKE_ARRAY
        t.args = x.map{|y| expr(y).to_pb}
      when Hash
        t.type = Term::TermType::MAKE_OBJ
        t.optargs = x.map{|k,v|
          ap = Term::AssocPair.new;
          if [Symbol, String].include? k.class
            ap.key = k.to_s
          else
            raise RqlDriverError, "Object keys must be strings or symbols." +
              "  (Got object `#{k.inspect}` of class `#{k.class}`.)"
          end
          ap.val = expr(v).to_pb
          ap
        }
      when Proc
        t = RQL.new.new_func(&x).to_pb
      else raise RqlDriverError, "r.expr can't handle #{x.inspect} of type #{x.class}"
      end

      return RQL.new(t)
    end

    def coerce(other)
      [RQL.new.expr(other), self]
    end
  end
end
