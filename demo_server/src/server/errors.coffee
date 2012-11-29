goog.provide('rethinkdb.server.Errors')

class RDBError extends Error

class RuntimeError extends RDBError
    constructor: (msg) ->
        @name = "Runtime error"
        @message = msg

class BadQuery extends RDBError
    constructor: (msg) ->
        @name = "Bad Query"
        @message = msg

class MissingAttribute extends RDBError
    constructor: (object, attribute) ->
        @name = "Missing attribute"
        @object = object
        @attribute = attribute
