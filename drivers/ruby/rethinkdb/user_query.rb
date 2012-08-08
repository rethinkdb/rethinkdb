module RethinkDB
  module RQL_Mixin
    def getattr a; S.new [:call, [:implicit_getattr, a], []]; end
    def pickattrs *a; S.new [:call, [:implicit_pickattrs, *a], []]; end
    def hasattr a; S.new [:call, [:implicit_hasattr, a], []]; end
    def [](ind); expr ind; end
    def db (x,y=nil); Tbl.new x,y; end
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
      when Symbol.hash     then S.new x.to_s[0]=='$'[0] ? var(x.to_s[1..-1]) : getattr(x)
      else raise TypeError, "term.expr can't handle '#{x.class()}'"
      end
    end
    def method_missing(m, *args, &block)
      return self.send(C.method_aliases[m], *args, &block) if C.method_aliases[m]
      if    P.enum_type(Builtin::BuiltinType, m) then S.new [:call, [m], args]
      elsif P.enum_type(Builtin::Comparison, m)  then S.new [:call, [:compare, m], args]
      elsif P.enum_type(Term::TermType, m)       then S.new [m, *args]
      else super(m, *args, &block)
      end
    end
  end
  module RQL; extend RQL_Mixin; end
end
