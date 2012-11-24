goog.provide('rethinkdb.server.Errors')

class RuntimeError extends Error
    constructor: (msg) ->
        @name = "Runtime error"
        @message = "Runtime error: #{msg}"
