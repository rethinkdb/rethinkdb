module RethinkDB
  class RQL
    @@gensym_cnt = 0
    def new_func(&b)
      args = (0...b.arity).map{@@gensym_cnt += 1}
      body = b.call(*(args.map{|i| RQL.new.var i}))
      RQL.new.func(args, body)
    end
    def method_missing(m, *a, &b)
      termtype = Term2::TermType.values[m.to_s.upcase.to_sym]
      unbound_if(!termtype, m)
      args = (@body ? [self] : []) + a + (b ? [new_func(&b)] : [])
      t = Term2.new
      t.type = termtype
      t.args = args.map{|x| RQL.new.expr(x).to_pb}
      return RQL.new t
    end
  end
end
