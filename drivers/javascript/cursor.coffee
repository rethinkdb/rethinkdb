err = require('./errors')
util = require('./util')
pb = require('./protobuf')

# Import some names to this namespace for convenience
ar = util.ar
varar = util.varar
aropt = util.aropt
deconstructDatum = util.deconstructDatum
mkErr = util.mkErr

# setImmediate is not defined in some browsers (including Chrome)
if not setImmediate?
    setImmediate = (cb) ->
        setTimeout cb, 0

class IterableResult
    hasNext: -> throw "Abstract Method"
    next: -> throw "Abstract Method"

    each: varar(1, 2, (cb, onFinished) ->
        unless typeof cb is 'function'
            throw new err.RqlDriverError "First argument to each must be a function."
        if onFinished? and typeof onFinished isnt 'function'
            throw new err.RqlDriverError "Optional second argument to each must be a function."

        brk = false
        n = =>
            if not brk and @hasNext()
                @next (err, row) =>
                    brk = (cb(err, row) is false)
                    n()
            else if onFinished?
                onFinished()
        n()
    )

    toArray: ar (cb) ->
        unless typeof cb is 'function'
            throw new err.RqlDriverError "Argument to toArray must be a function."

        arr = []
        if not @hasNext()
            cb null, arr
        @each (err, row) =>
            if err?
                cb err
            else
                arr.push(row)

            if not @hasNext()
                cb null, arr

class Cursor extends IterableResult
    stackSize: 100

    constructor: (conn, token, opts, root) ->
        @_conn = conn
        @_token = token
        @_opts = opts
        @_root = root

        @_responses = []
        @_responseIndex = 0
        @_outstandingRequests = 1
        @_iterations = 0
        @_endFlag = false
        @_contFlag = false
        @_cont = null
        @_cbQueue = []

    _addResponse: (response) ->
        @_responses.push response
        @_outstandingRequests -= 1

        pb.ResponseTypeSwitch(response, {
            "SUCCESS_PARTIAL": =>
                @_endFlag = false
            },
                => @_endFlag = true
        )

        @_contFlag = false
        @_promptNext()
        @

    _getCallback: ->
        @_iterations += 1
        cb = @_cbQueue.shift()

        if @_iterations % @stackSize is @stackSize - 1
            immediateCb = ((err, row) -> setImmediate -> cb(err, row))
            return immediateCb
        else
            return cb

    _handleRow: ->
        response = @_responses[0]
        row = deconstructDatum(response.response[@_responseIndex], @_opts)
        cb = @_getCallback()

        @_responseIndex += 1

        # If we're done with this response, discard it
        if @_responseIndex is response.response.length
            @_responses.shift()
            @_responseIndex = 0

        cb null, row

    _promptNext: ->
        # If there are no more waiting callbacks, just wait until the next event
        while @_cbQueue[0]?
            # If there's no more data let's notify the waiting callback
            if not @hasNext()
                cb = @_getCallback()
                cb new err.RqlDriverError "No more rows in the cursor."
            else
                # Try to get a row out of the responses
                response = @_responses[0]

                if @_responses.length is 1
                    # We're low on data, prebuffer
                    @_promptCont()

                    if !@_endFlag && response.response? && @_responseIndex is response.response.length - 1
                        # Only one row left and we aren't at the end of the stream, we have to hold
                        #  onto this so we know if there's more data for hasNext
                        return

                # Error responses are not discarded, and the error will be sent to all future callbacks
                pb.ResponseTypeSwitch(response, {
                    "SUCCESS_PARTIAL": =>
                        @_handleRow()
                    ,"SUCCESS_SEQUENCE": =>
                        if response.response.length is 0
                            @_responses.shift()
                        else
                            @_handleRow()
                    ,"COMPILE_ERROR": =>
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlCompileError, response, @_root)
                    ,"CLIENT_ERROR": =>
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlClientError, response, @_root)
                    ,"RUNTIME_ERROR": =>
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlRuntimeError, response, @_root)
                    },
                        =>
                            @_responses.shift()
                            cb = @_getCallback()
                            cb new err.RqlDriverError "Unknown response type for cursor"
                )

    _promptCont: ->
        # Let's ask the server for more data if we haven't already
        if !@_contFlag && !@_endFlag
            @_contFlag = true
            @_outstandingRequests += 1
            @_conn._continueQuery(@_token)


    ## Implement IterableResult

    hasNext: ar () -> @_responses[0]? && @_responses[0].response.length > 0

    next: ar (cb) ->
        nextCbCheck(cb)
        @_cbQueue.push cb
        @_promptNext()

    close: ar () ->
        unless @_endFlag
            @_outstandingRequests += 1
            @_conn._endQuery(@_token)

    toString: ar () -> "[object Cursor]"

# Used to wrap array results so they support the same iterable result
# API as cursors.

class ArrayResult extends IterableResult
    # How many many results we return before resetting the stack with setImmediate
    # The higher the better, but if it's too hight users will hit a maximum call stack size
    stackSize: 100

    # We store @__index as soon as the user starts using the cursor interface
    hasNext: ar () ->
        if not @__index?
            @__index = 0
        @__index < @length

    next: ar (cb) ->
        nextCbCheck(cb)

        # If people call next
        if not @__index?
            @__index = 0

        if @hasNext() is true
            self = @
            if self.__index%@stackSize is @stackSize-1
                # Reset the stack
                setImmediate ->
                    cb(null, self[self.__index++])
            else
                cb(null, self[self.__index++])
        else
            cb new err.RqlDriverError "No more rows in the cursor."

    toArray: ar (cb) ->
        # IterableResult.toArray would create a copy
        if @__index?
            cb(null, @.slice(@__index, @.length))
        else
            cb(null, @)

    close: ->
        return @

    makeIterable: (response) ->
        response.__proto__ = {}
        for name, method of ArrayResult.prototype
            if name isnt 'constructor'
                response.__proto__[name] = method
        response.__proto__.__proto__ = [].__proto__
        response

nextCbCheck = (cb) ->
    unless typeof cb is 'function'
        throw new err.RqlDriverError "Argument to next must be a function."

module.exports.deconstructDatum = deconstructDatum
module.exports.Cursor = Cursor
module.exports.makeIterable = ArrayResult::makeIterable
