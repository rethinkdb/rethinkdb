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
        joinTree tree

    composeTerm = (term) ->
        args = (composeTerm arg for arg in term.args)
        term.compose(args)

    printCarrots = (term, frames) ->
        tree = composeCarrots(term, frames)
        joinTree tree

    composeCarrots = (term, frames) ->
        argNum = frames.shift()
        unless argNum? then argNum = -1
        args = for arg,i in term.args
                   if i == argNum
                       composeCarrots(arg, frames)
                   else
                       composeTerm(arg)
        if argNum >= 0
            term.compose(args)
        else
            carrotify(term.compose(args))

    carrotify = (tree) -> (joinTree tree).replace(/[^\^]/g, '^')
    
    joinTree = (tree) ->
        str = ''
        for term in tree
            if goog.isArray term
                str += joinTree term
            else
                str += term
        return str

class RuntimeError extends ServerError
    errName = "RuntimeError"
