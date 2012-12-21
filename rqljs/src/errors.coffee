goog.provide('rethinkdb.Errors')

class RDBError extends Error
    constructor: ->

class ServerError extends RDBError
	constructor: (msg) ->
		@name = "Server Error"
		@message = msg

class RuntimeError extends RDBError
    constructor: (msg) ->
        @name = "Runtime error"
        @message = msg
		@backtrace = []

class BadQuery extends RDBError
    constructor: (msg) ->
        @name = "Bad Query"
        @message = msg
