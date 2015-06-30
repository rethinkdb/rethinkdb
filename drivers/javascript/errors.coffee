class RqlDriverError extends Error
    constructor: (msg) ->
        @name = @constructor.name
        @msg = msg
        @message = msg
        if Error.captureStackTrace?
            Error.captureStackTrace @, @

class RqlServerError extends Error
    constructor: (msg, term, frames) ->
        @name = @constructor.name
        @msg = msg
        @frames = frames[0..]
        if term?
            if msg[msg.length-1] is '.'
                @message = "#{msg.slice(0, msg.length-1)} in:\n#{RqlQueryPrinter::printQuery(term)}\n#{RqlQueryPrinter::printCarrots(term, frames)}"
            else
                @message = "#{msg} in:\n#{RqlQueryPrinter::printQuery(term)}\n#{RqlQueryPrinter::printCarrots(term, frames)}"
        else
            @message = "#{msg}"
        if Error.captureStackTrace?
            Error.captureStackTrace @, @

class RqlRuntimeError extends RqlServerError

class RqlCompileError extends RqlServerError
class RqlClientError extends RqlServerError

class RqlInternalError extends RqlRuntimeError
class RqlResourceError extends RqlRuntimeError
class RqlLogicError extends RqlRuntimeError
class RqlNonExistenceError extends RqlRuntimeError

class RqlOpError extends RqlRuntimeError
class RqlOpFailedError extends RqlOpError
class RqlOpIndeterminateError extends RqlOpError

class RqlUserError extends RqlRuntimeError

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
        if frames.length is 0
            tree = [carrotify(composeTerm(term))]
        else
            tree = composeCarrots(term, frames)
        (joinTree tree).replace(/[^\^]/g, ' ')

    composeCarrots = (term, frames) ->
        frame = frames.shift()

        args = for arg,i in term.args
                    if frame is i
                        composeCarrots(arg, frames)
                    else
                        composeTerm(arg)

        optargs = {}
        for own key,arg of term.optargs
            if frame is key
                optargs[key] = composeCarrots(arg, frames)
            else
                optargs[key] = composeTerm(arg)

        if frame?
            term.compose(args, optargs)
        else
            carrotify(term.compose(args, optargs))

    carrotMarker = {}

    carrotify = (tree) -> [carrotMarker, tree]

    joinTree = (tree) ->
        str = ''
        for term in tree
            if Array.isArray term
                if term.length == 2 and term[0] is carrotMarker
                    str += (joinTree term[1]).replace(/./g, '^')
                else
                    str += joinTree term
            else
                str += term
        return str


module.exports = {
    RqlDriverError
    RqlRuntimeError

    RqlCompileError
    RqlClientError

    RqlInternalError
    RqlResourceError
    RqlLogicError
    RqlNonExistenceError

    RqlOpError
    RqlOpFailedError
    RqlOpIndeterminateError

    RqlUserError

    printQuery: RqlQueryPrinter::printQuery
}
