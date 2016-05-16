module RethinkDB
  RqlError = ReqlError = Class.new(RuntimeError)

  RqlRuntimeError = ReqlRuntimeError = Class.new(ReqlError)
  RqlInternalError = ReqlInternalError = Class.new(ReqlRuntimeError)
  RqlResourceLimitError = ReqlResourceLimitError = Class.new(ReqlRuntimeError)
  RqlQueryLogicError = ReqlQueryLogicError = Class.new(ReqlRuntimeError)
  RqlNonExistenceError = ReqlNonExistenceError = Class.new(ReqlQueryLogicError)
  RqlAvailabilityError = ReqlAvailabilityError = Class.new(ReqlRuntimeError)
  RqlOpFailedError = ReqlOpFailedError = Class.new(ReqlAvailabilityError)
  RqlOpIndeterminateError = ReqlOpIndeterminateError = Class.new(ReqlAvailabilityError)
  RqlUserError = ReqlUserError = Class.new(ReqlRuntimeError)
  ReqlPermissionError = Class.new(ReqlRuntimeError)

  RqlDriverError = ReqlDriverError = Class.new(ReqlError)
  RqlAuthError = ReqlAuthError = Class.new(ReqlDriverError)
  RqlCompileError = ReqlCompileError = Class.new(ReqlError)
  ReqlServerCompileError = Class.new(ReqlCompileError)
  ReqlDriverCompileError = Class.new(ReqlCompileError)
end
