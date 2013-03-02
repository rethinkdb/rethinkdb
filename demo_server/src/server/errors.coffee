goog.provide('rethinkdb.server.Errors')

class RDBError extends Error
    constructor: ->

class RuntimeError extends RDBError
    constructor: (msg) ->
        @name = "Runtime error"
        @message = msg

class BadQuery extends RDBError
    constructor: (msg) ->
        @name = "Bad Query"
        @message = msg
