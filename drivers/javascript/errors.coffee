class RqlDriverError extends Error
    constructor: (msg) ->
        @name = @constructor.name
        @msg = msg
        @message = msg

class RqlServerError extends Error
    constructor: (msg, term, frames) ->
        @name = @constructor.name
        @msg = msg
        @frames = frames[0..]
        if term?
            @message = "#{msg} in:\n#{RqlQueryPrinter::printQuery(term)}\n#{RqlQueryPrinter::printCarrots(term, frames)}"
        else
            @message = "#{msg}"

class RqlRuntimeError extends RqlServerError

class RqlCompileError extends RqlServerError

class RqlClientError extends RqlServerError

class RqlQueryPrinter
    printQuery: (term) ->
        tree = composeTerm(term)
        joinTree tree

    composeTerm = (term) ->
        args = (composeTerm arg for arg in term.args)
        optargs = {}
        for own key,arg of term.optargs
            optargs[key] = composeTerm(arg)
        term.compose(args, optargs)

    printCarrots: (term, frames) ->
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
        for own key,arg of term.optargs
            optargs[key] = if key == argNum
                             composeCarrots(arg, frames)
                           else
                             composeTerm(arg)
        if argNum != -1
            term.compose(args, optargs)
        else
            carrotify(term.compose(args, optargs))

    carrotify = (tree) -> (joinTree tree).replace(/./g, '^')

    joinTree = (tree) ->
        str = ''
        for term in tree
            if Array.isArray term
                str += joinTree term
            else
                str += term
        return str

module.exports.RqlDriverError = RqlDriverError
module.exports.RqlRuntimeError = RqlRuntimeError
module.exports.RqlCompileError = RqlCompileError
module.exports.RqlClientError = RqlClientError
module.exports.printQuery = RqlQueryPrinter::printQuery
