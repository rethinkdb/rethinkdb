class ReqlError extends Error
    constructor: (msg) ->
        @name = @constructor.name
        @msg = msg
        @message = msg
        if Error.captureStackTrace?
            Error.captureStackTrace @, @

class ReqlCompileError extends ReqlError

class ReqlDriverCompileError extends ReqlCompileError

class ReqlServerCompileError extends ReqlCompileError

class ReqlDriverError extends ReqlError

class ReqlAuthError extends ReqlDriverError

class ReqlRuntimeError extends ReqlError
    constructor: (msg, term, frames) ->
        @name = @constructor.name
        @msg = msg
        @frames = frames[0..]
        if term?
            if msg[msg.length-1] is '.'
                @message = "#{msg.slice(0, msg.length-1)} in:\n#{ReqlQueryPrinter::printQuery(term)}\n#{ReqlQueryPrinter::printCarrots(term, frames)}"
            else
                @message = "#{msg} in:\n#{ReqlQueryPrinter::printQuery(term)}\n#{ReqlQueryPrinter::printCarrots(term, frames)}"
        else
            @message = "#{msg}"
        if Error.captureStackTrace?
            Error.captureStackTrace @, @

class ReqlQueryLogicError extends ReqlRuntimeError
class ReqlNonExistenceError extends ReqlQueryLogicError
class ReqlResourceLimitError extends ReqlRuntimeError
class ReqlUserError extends ReqlRuntimeError
class ReqlInternalError extends ReqlRuntimeError
class ReqlTimeoutError extends ReqlError
class ReqlAvailabilityError extends ReqlRuntimeError
class ReqlOpFailedError extends ReqlAvailabilityError
class ReqlOpIndeterminateError extends ReqlAvailabilityError


class ReqlQueryPrinter
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
    ReqlError
    ReqlCompileError
    RqlCompileError: ReqlCompileError
    ReqlServerCompileError
    ReqlDriverCompileError

    RqlClientError: ReqlDriverError

    ReqlRuntimeError
    RqlRuntimeError: ReqlRuntimeError
    RqlServerError: ReqlRuntimeError  # no direct equivalent

    ReqlQueryLogicError
    ReqlNonExistenceError

    ReqlResourceLimitError
    ReqlUserError
    ReqlInternalError
    ReqlTimeoutError

    ReqlAvailabilityError
    ReqlOpFailedError
    ReqlOpIndeterminateError

    ReqlDriverError
    RqlDriverError: ReqlDriverError
    ReqlAuthError

    printQuery: ReqlQueryPrinter::printQuery
}
