goog.provide('rethinkdb.server.Errors')

class RDBError extends Error

class RuntimeError extends RDBError
    constructor: (msg) ->
        @name = "Runtime error"
        @message = msg
