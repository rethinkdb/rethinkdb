module RethinkDB
  RqlError        = Class.new(RuntimeError)
  RqlRuntimeError = Class.new(RqlError)
  RqlDriverError  = Class.new(RqlError)
  RqlCompileError = Class.new(RqlError)
end
