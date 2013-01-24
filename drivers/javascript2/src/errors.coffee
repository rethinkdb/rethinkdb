goog.provide("rethinkdb.errors")

goog.require("rethinkdb.base")

class DriverError extends Error
    constructor: (msg) ->
        @name = "DriverError"
        @message = msg

class RuntimeError extends Error
    constructor: (msg) ->
        @name = "RuntimeError"
        @message = msg

AbstractMethod = new DriverError "Abstract method"
