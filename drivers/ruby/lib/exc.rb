module RethinkDB
  ReqlError = RqlError = Class.new(RuntimeError)

  ReqlRuntimeError = RqlRuntimeError = Class.new(ReqlError)
  ReqlInternalError = RqlInternalError = Class.new(ReqlRuntimeError)
  ReqlResourceLimitError = RqlResourceLimitError = Class.new(ReqlRuntimeError)
  ReqlQueryLogicError = RqlQueryLogicError = Class.new(ReqlRuntimeError)
  ReqlNonExistenceError = RqlNonExistenceError = Class.new(ReqlQueryLogicError)
  ReqlAvailabilityError = RqlAvailabilityError = Class.new(ReqlRuntimeError)
  ReqlOpFailedError = RqlOpFailedError = Class.new(ReqlAvailabilityError)
  ReqlOpIndeterminateError = RqlOpIndeterminateError = Class.new(ReqlAvailabilityError)
  ReqlUserError = RqlUserError = Class.new(ReqlRuntimeError)

  ReqlDriverError = RqlDriverError = Class.new(ReqlError)
  ReqlAuthError = RqlAuthError = Class.new(ReqlDriverError)
  ReqlCompileError = RqlCompileError = Class.new(ReqlError)
  ReqlServerCompileError = Class.new(ReqlCompileError)
  ReqlDriverCompileError = Class.new(ReqlCompileError)
end
