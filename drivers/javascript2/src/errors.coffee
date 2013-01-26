goog.provide("rethinkdb.errors")

goog.require("rethinkdb.base")

class DriverError extends Error
    constructor: (msg) ->
        @name = "DriverError"
        @message = msg

class ServerError extends Error
    constructor: (msg, term, frames) ->
        @name = "RuntimeError"
        @message = "#{msg} in:\n#{printQuery(term)}\n#{printCarrots(term, frames)}"

    printQuery = (term) ->
        tree = composeTerm(term)
        #joinTree tree

    printCarrots = (term, frames) ->
        ""

    composeTerm = (term) ->
        args = (composeTerm arg for arg in term.args)
        term.compose(args)

    #joinTree = (tree) ->
    #    for term in tree
    #        if 

class RuntimeError extends ServerError
    errName = "RuntimeError"
