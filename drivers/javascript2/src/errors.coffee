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
        optargs = {}
        for own key,arg in term.optargs
            optargs[key] = composeTerm(arg)
        term.compose(args, optargs)

    printCarrots = (term, frames) ->
        tree = composeCarrots(term, frames)
        (joinTree tree).replace(/[^\^]/g, ' ')

    composeCarrots = (term, frames) ->
        argNum = frames.shift()
        unless argNum? then argNum = -1

        args = for arg,i in term.args
                    if i == argNum
                        composeCarrots(arg, frames)
                    else
                        composeTerm(arg)

        optargs = {}
        for own key,arg in term.optargs
            optargs[key] = if key == argNum
                             composeCarrots(arg, frames)
                           else
                             comoseTerm(arg)
        if argNum >= 0
            term.compose(args, optargs)
        else
            carrotify(term.compose(args, optargs))

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
