err = require('./errors')
util = require('./util')

protoResponseType = require('./proto-def').Response.ResponseType
Promise = require('bluebird')

# Import some names to this namespace for convenience
ar = util.ar
varar = util.varar
aropt = util.aropt
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

    toArray: varar 0, 1, (cb) ->
        fn = (cb) =>
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

        if typeof cb is 'function'
            fn(cb)
        else if cb is undefined
            p = new Promise (resolve, reject) =>
                cb = (err, result) ->
                    if err?
                        reject(err)
                    else
                        resolve(result)
                fn(cb)
            return p
        else
            throw new err.RqlDriverError "First argument to `toArray` must be a function or undefined."

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
        @_endFlag = !(response.t is protoResponseType.SUCCESS_PARTIAL)
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
        row = util.recursivelyConvertPseudotype(response.r[@_responseIndex], @_opts)
        cb = @_getCallback()

        @_responseIndex += 1

        # If we're done with this response, discard it
        if @_responseIndex is response.r.length
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

                    if !@_endFlag && response.r? && @_responseIndex is response.r.length - 1
                        # Only one row left and we aren't at the end of the stream, we have to hold
                        #  onto this so we know if there's more data for hasNext
                        return

                # Error responses are not discarded, and the error will be sent to all future callbacks
                switch response.t
                    when protoResponseType.SUCCESS_PARTIAL
                        @_handleRow()
                    when protoResponseType.SUCCESS_SEQUENCE
                        if response.r.length is 0
                            @_responses.shift()
                        else
                            @_handleRow()
                    when protoResponseType.COMPILE_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlCompileError, response, @_root)
                    when protoResponseType.CLIENT_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlClientError, response, @_root)
                    when protoResponseType.RUNTIME_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlRuntimeError, response, @_root)
                    else
                        @_responses.shift()
                        cb = @_getCallback()
                        cb new err.RqlDriverError "Unknown response type for cursor"

    _promptCont: ->
        # Let's ask the server for more data if we haven't already
        if !@_contFlag && !@_endFlag
            @_contFlag = true
            @_outstandingRequests += 1
            @_conn._continueQuery(@_token)


    ## Implement IterableResult

    hasNext: ar () -> @_responses[0]? && @_responses[0].r.length > 0

    next: varar 0, 1, (cb) ->
        fn = (cb) =>
            @_cbQueue.push cb
            @_promptNext()

        if typeof cb is "function"
            fn(cb)
        else if cb is undefined
            p = new Promise (resolve, reject) ->
                cb = (err, result) ->
                    if (err)
                        reject(err)
                    else
                        resolve(result)
                fn(cb)
            return p
        else
            throw new err.RqlDriverError "First argument to `next` must be a function or undefined."

    close: ar () ->
        unless @_endFlag
            @_outstandingRequests += 1
            @_conn._endQuery(@_token)

    toString: ar () -> "[object Cursor]"


class Feed
    stackSize: 100

    constructor: (conn, token, opts, root) ->
        @_conn = conn
        @_token = token
        @_opts = opts
        @_root = root # current query

        @_responses = []
        @_responseIndex = 0
        @_outstandingRequests = 1 # Because we haven't add the response yet
        @_iterations = 0
        @_endFlag = false
        @_contFlag = false
        @_cont = null
        @_cbQueue = []

    bufferEmpty: ->
        @_responses.length is 0 or @_responses[0].r.length <= @_responseIndex

    _addResponse: (response) ->
        @_responses.push response
        @_outstandingRequests -= 1
        @_endFlag = response.t isnt protoResponseType.SUCCESS_FEED
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
        row = util.recursivelyConvertPseudotype(response.r[@_responseIndex], @_opts)
        cb = @_getCallback()

        @_responseIndex += 1

        # If we're done with this response, discard it
        if @_responseIndex is response.r.length
            @_responses.shift()
            @_responseIndex = 0

        cb null, row

    _promptNext: ->
        while @_cbQueue[0]?
            if @bufferEmpty() is true
                if @_responses.length is 1
                    @_responses.shift()
                    
                if @_responses.length <= 1
                    @_promptCont()

                return
            else
                # Try to get a row out of the responses
                response = @_responses[0]

                if @_responses.length <= 1
                    @_promptCont()

                # Error responses are not discarded, and the error will be sent to all future callbacks
                switch response.t
                    when protoResponseType.SUCCESS_FEED
                        @_handleRow()
                    when protoResponseType.SUCCESS_SEQUENCE
                        if response.r.length is 0
                            @_responses.shift()
                        else
                            @_handleRow()
                    when protoResponseType.COMPILE_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlCompileError, response, @_root)
                    when protoResponseType.CLIENT_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlClientError, response, @_root)
                    when protoResponseType.RUNTIME_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.RqlRuntimeError, response, @_root)
                    else
                        @_responses.shift()
                        cb = @_getCallback()
                        cb new err.RqlDriverError "Unknown response type for cursor"

    _promptCont: ->
        # Let's ask the server for more data if we haven't already
        if !@_contFlag && !@_endFlag
            @_contFlag = true
            @_outstandingRequests += 1
            @_conn._continueQuery(@_token)

    hasNext: ->
        throw new err.RqlDriverError "hasNext is not available for feeds."
    toArray: ->
        throw new err.RqlDriverError "toArray is not available for feeds."

    next: (cb) ->
        fn = (cb) =>
            @_cbQueue.push cb
            @_promptNext()

        if typeof cb is "function"
            fn(cb)
        else if cb is undefined
            p = new Promise (resolve, reject) ->
                cb = (err, result) ->
                    if (err)
                        reject(err)
                    else
                        resolve(result)
                fn(cb)
            return p
        else
            throw new err.RqlDriverError "First argument to `next` must be a function or undefined."


    close: ->
        unless @_endFlag
            @_outstandingRequests += 1
            @_conn._endQuery(@_token)

    toString: ar () -> "[object Feed]"

    each: ->
        #TODO

    on: ->
        #TODO
    off: ->
        #TODO

    #TODO other EventEmitter methods


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

    next: varar 0, 1, (cb) ->
        fn = (cb) =>

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

        if typeof cb is "function"
            fn(cb)
        else
            p = new Promise (resolve, reject) ->
                cb = (err, result) ->
                    if (err)
                        reject(err)
                    else
                        resolve(result)
                fn(cb)


    toArray: varar 0, 1, (cb) ->
        fn = (cb) =>
            # IterableResult.toArray would create a copy
            if @__index?
                cb(null, @.slice(@__index, @.length))
            else
                cb(null, @)

        if typeof cb is "function"
            fn(cb)
        else
            p = new Promise (resolve, reject) ->
                cb = (err, result) ->
                    if (err)
                        reject(err)
                    else
                        resolve(result)
                fn(cb)
            return p


    close: ->
        return @

    makeIterable: (response) ->
        response.__proto__ = {}
        for name, method of ArrayResult.prototype
            if name isnt 'constructor'
                response.__proto__[name] = method
        response.__proto__.__proto__ = [].__proto__
        response

module.exports.Cursor = Cursor
module.exports.Feed = Feed
module.exports.makeIterable = ArrayResult::makeIterable
