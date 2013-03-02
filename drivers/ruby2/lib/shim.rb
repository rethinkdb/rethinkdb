module RethinkDB
  module Shim
    def self.datum_to_native d
      dt = Datum::DatumType
      case d.type
      when dt::R_NUM then d.r_num
      when dt::R_STR then d.r_str
      when dt::R_BOOL then d.r_bool
      when dt::R_NULL then nil
      when dt::R_ARRAY then d.r_array.map{|d2| datum_to_native d2}
      when dt::R_OBJECT then Hash[d.r_object.map{|x| [x.key, datum_to_native(x.val)]}]
      else raise RuntimeError, "Unimplemented."
      end
    end

    def self.response_to_native r
      rt = Response2::ResponseType
      if r.backtrace
        bt = r.backtrace.frames.map {|x|
          x.type == Frame::FrameType::POS ? x.pos : x.opt
        }
      else
        bt = []
      end
      case r.type
      when rt::SUCCESS_ATOM then datum_to_native(r.response[0])
      when rt::SUCCESS_PARTIAL then r.response.map{|d| datum_to_native(d)}
      when rt::SUCCESS_SEQUENCE then r.response.map{|d| datum_to_native(d)}
      when rt::RUNTIME_ERROR then
        raise RuntimeError, "#{r.response[0].r_str}\nBacktrace: #{bt.inspect}"
      when rt::COMPILE_ERROR then # TODO: remove?
        raise RuntimeError, "#{r.response[0].r_str}\nBacktrace: #{bt.inspect}"
      else raise RuntimeError, "Unexpected response: #{r.inspect}"
      end
    end

    def self.native_to_datum_term x
      dt = Datum::DatumType
      d = Datum.new
      case x.class.hash
      when Fixnum.hash     then d.type = dt::R_NUM;  d.r_num = x
      when Float.hash      then d.type = dt::R_NUM;  d.r_num = x
      when String.hash     then d.type = dt::R_STR;  d.r_str = x
      when Symbol.hash     then d.type = dt::R_STR;  d.r_str = x.to_s
      when TrueClass.hash  then d.type = dt::R_BOOL; d.r_bool = x
      when FalseClass.hash then d.type = dt::R_BOOL; d.r_bool = x
      when NilClass.hash   then d.type = dt::R_NULL
      else raise RuntimeError, "UNREACHABLE"
      end
      t = Term2.new
      t.type = Term2::TermType::DATUM
      t.datum = d
      return t
    end
  end

  class RQL
    def to_pb; @body; end

    def expr(x)
      unbound_if @body
      return x if x.class == RQL
      datum_types = [Fixnum, Float, String, Symbol, TrueClass, FalseClass, NilClass]
      if datum_types.map{|y| y.hash}.include? x.class.hash
        return RQL.new(Shim.native_to_datum_term(x))
      end

      t = Term2.new
      case x.class.hash
      when Array.hash
        t.type = Term2::TermType::MAKE_ARRAY
        t.args = x.map{|y| expr(y).to_pb}
      when Hash.hash
        t.type = Term2::TermType::MAKE_OBJ
        t.optargs = x.map{|k,v| ap = Term2::AssocPair.new;
          ap.key = k.to_s; ap.val = expr(v).to_pb; ap}
      when Proc.hash
        t = RQL.new.new_func(&x).to_pb
      else raise RuntimeError, "r.expr can't handle #{x.inspect} of type #{x.class}"
      end

      return RQL.new(t)
    end

  end
end
