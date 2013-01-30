goog.provide('rethinkdb.Errors')

class RDBError extends Error

class ServerError extends RDBError
    constructor: (msg) ->
        @name = "Server Error"
        @message = msg
        @backtrace = []

class RuntimeError extends RDBError
    constructor: (msg) ->
        @name = "Runtime error"
        @message = msg
        @backtrace = []

class TypeError extends RuntimeError
    constructor: (msg) ->
        @name = "TypeError"
        @message = msg
        @backtrace = []

class BadQuery extends RDBError
    constructor: (msg) ->
        @name = "Bad Query"
        @message = msg
        @backtrace = []
