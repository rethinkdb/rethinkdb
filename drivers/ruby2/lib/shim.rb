module RethinkDB
  class RQL
    attr_accessor :body
    def to_pb; @body; end
    def initialize(body = nil)
      @body = body
    end

    def to_datum_term x
      dt = Datum::DatumType
      d = Datum.new
      case x.class.hash
      when Fixnum.hash then d.type = dt::R_NUM; d.r_num = x
      when Float.hash then d.type = dt::R_NUM; d.r_num = x
      when String.hash then d.type = dt::R_STR; d.r_str = x
      when Symbol.hash then d.type = dt::R_STR; d.r_str = x.to_s
      when TrueClass.hash then d.type = dt::R_BOOL; d.r_bool = x
      when FalseClass.hash then d.type = dt::R_BOOL; d.r_bool = x
      when NilClass.hash then d.type = dt::R_NULL
      else raise RuntimeError, "UNREACHABLE"
      end
      t = Term2.new
      t.type = Term2::TermType::DATUM
      t.datum = d
      return t
    end
    def expr(x)
      unbound_if @body
      return x if x.class == Term2
      datum_types = [Fixnum, Float, String, Symbol, TrueClass, FalseClass, NilClass]
      if datum_types.map{|y| y.hash}.include? x.class.hash
        return RQL.new(to_datum_term(x))
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
      else raise RuntimeError, "r.expr can't handle #{x.inspect} of type #{x.class}"
      end

      return RQL.new(t)
    end
  end
end
