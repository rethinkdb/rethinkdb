goog.provide('rethinkdb.Errors')

class RqlError extends Error

class RqlServerError extends RqlError
    constructor: (msg) ->
        @name = "RqlServerError"
        @message = msg
        @backtrace = []

class RqlClientError extends RqlError
    constructor: (msg) ->
        @name = "RqlClientError"
        @message = msg
        @backtrace = []

class RqlCompileError extends RqlError
    constructor: (msg) ->
        @name = "RqlCompileError"
        @message = msg
        @backtrace = []

class RqlRuntimeError extends RqlError
    constructor: (msg) ->
        @name = "RqlRuntimeError"
        @message = msg
        @backtrace = []
