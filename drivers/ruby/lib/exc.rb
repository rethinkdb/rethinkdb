module RethinkDB
  RqlError                = Class.new(RuntimeError)

  RqlRuntimeError         = Class.new(RqlError)
  RqlInternalError        = Class.new(RqlRuntimeError)
  RqlResourceError        = Class.new(RqlRuntimeError)
  RqlLogicError           = Class.new(RqlRuntimeError)
  RqlNonExistenceError    = Class.new(RqlLogicError)
  RqlOpError              = Class.new(RqlRuntimeError)
  RqlOpFailedError        = Class.new(RqlOpError)
  RqlOpIndeterminateError = Class.new(RqlOpError)
  RqlUserError            = Class.new(RqlRuntimeError)

  RqlDriverError          = Class.new(RqlError)
  RqlCompileError         = Class.new(RqlError)
end
