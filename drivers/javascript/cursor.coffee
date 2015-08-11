err = require('./errors')
util = require('./util')

protoResponseType = require('./proto-def').Response.ResponseType
Promise = require('bluebird')
EventEmitter = require('events').EventEmitter

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
    stackSize: 100

    constructor: (conn, token, opts, root) ->
        @_conn = conn
        @_token = token
        @_opts = opts
        @_root = root # current query

        @_responses = []
        @_responseIndex = 0
        @_outstandingRequests = 1 # Because we haven't added the response yet
        @_iterations = 0
        @_endFlag = false
        @_contFlag = false
        @_closeAsap = false
        @_cont = null
        @_cbQueue = []
        @_closeCb = null
        @_closeCbPromise = null

        @next = @_next
        @each = @_each

    _addResponse: (response) ->
        if response.t is @_type or response.t is protoResponseType.SUCCESS_SEQUENCE
            # We push a "ok" response only if it's not empty
            if response.r.length > 0
                @_responses.push response
        else
            @_responses.push response


        @_outstandingRequests -= 1
        if response.t isnt @_type
            # We got an error or a SUCCESS_SEQUENCE
            @_endFlag = true

            if @_closeCb?
                switch response.t
                    when protoResponseType.COMPILE_ERROR
                        @_closeCb mkErr(err.ReqlCompileError, response, @_root)
                    when protoResponseType.CLIENT_ERROR
                        @_closeCb mkErr(err.ReqlClientError, response, @_root)
                    when protoResponseType.RUNTIME_ERROR
                        @_closeCb mkErr(util.errorClass(response.e), response, @_root)
                    else
                        @_closeCb()

        @_contFlag = false
        if @_closeAsap is false
            @_promptNext()
        else
            @close @_closeCb
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

    bufferEmpty: ->
        @_responses.length is 0 or @_responses[0].r.length <= @_responseIndex

    _promptNext: ->
        # If there are no more waiting callbacks, just wait until the next event
        while @_cbQueue[0]?
            if @bufferEmpty() is true
                # We prefetch things here, set `is 0` to avoid prefectch
                if @_endFlag is true
                    cb = @_getCallback()
                    cb new err.ReqlDriverError "No more rows in the cursor."
                else if @_responses.length <= 1
                    @_promptCont()

                return
            else

                # Try to get a row out of the responses
                response = @_responses[0]

                if @_responses.length is 1
                    # We're low on data, prebuffer
                    @_promptCont()

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
                        cb mkErr(err.ReqlCompileError, response, @_root)
                    when protoResponseType.CLIENT_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(err.ReqlClientError, response, @_root)
                    when protoResponseType.RUNTIME_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        errType = util.errorClass(response.e)
                        cb mkErr(errType, response, @_root)
                    else
                        @_responses.shift()
                        cb = @_getCallback()
                        cb new err.ReqlDriverError "Unknown response type for cursor"

    _promptCont: ->
        # Let's ask the server for more data if we haven't already
        if (not @_contFlag) and (not @_endFlag) and @_conn.isOpen()
            @_contFlag = true
            @_outstandingRequests += 1
            @_conn._continueQuery(@_token)


    ## Implement IterableResult
    hasNext: ->
        throw new err.ReqlDriverError "The `hasNext` command has been removed since 1.13. Use `next` instead."

    _next: varar 0, 1, (cb) ->
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
            throw new err.ReqlDriverError "First argument to `next` must be a function or undefined."


    close: varar 0, 1, (cb) ->
        if @_closeCbPromise?
            if @_closeCbPromise.isPending()
                # There's an existing promise and it hasn't resolved
                # yet, so we chain this callback onto it.
                @_closeCbPromise = @_closeCbPromise.nodeify(cb)
            else
                # The existing promise has been fulfilled, so we chuck
                # it out and replace it with a Promise that resolves
                # immediately with the callback.
                @_closeCbPromise = Promise.resolve().nodeify(cb)
        else # @_closeCbPromise not set
            if @_endFlag
                # We are ended and this is the first time close() was
                # called. Just return a promise that resolves
                # immediately.
                @_closeCbPromise = Promise.resolve().nodeify(cb)
            else
                # We aren't ended, and we need to. Create a promise
                # that's resolved when the END query is acknowledged.
                @_closeCbPromise = new Promise((resolve, reject) =>
                    @_closeCb = (err) =>
                        # Clear all callbacks for outstanding requests
                        while @_cbQueue.length > 0
                            @_cbQueue.shift()
                        # The connection uses _outstandingRequests to see
                        # if it should remove the token for this
                        # cursor. This states unambiguously that we don't
                        # care whatever responses return now.
                        @_outstandingRequests = 0
                        if (err)
                            reject(err)
                        else
                            resolve()
                    @_closeAsap = true
                    @_outstandingRequests += 1
                    @_conn._endQuery(@_token)
                ).nodeify(cb)
        return @_closeCbPromise

    _each: varar(1, 2, (cb, onFinished) ->
        unless typeof cb is 'function'
            throw new err.ReqlDriverError "First argument to each must be a function."
        if onFinished? and typeof onFinished isnt 'function'
            throw new err.ReqlDriverError "Optional second argument to each must be a function."

        stopFlag = false
        self = @
        nextCb = (err, data) =>
            if stopFlag isnt true
                if err?
                    if err.message is 'No more rows in the cursor.'
                        if onFinished?
                            onFinished()
                    else
                        cb(err)
                else
                    stopFlag = cb(null, data) is false
                    @_next nextCb
            else if onFinished?
                onFinished()
        @_next nextCb
    )

    toArray: varar 0, 1, (cb) ->
        fn = (cb) =>
            arr = []
            eachCb = (err, row) =>
                if err?
                    cb err
                else
                    arr.push(row)

            onFinish = (err, ar) =>
                cb null, arr

            @each eachCb, onFinish

        if cb? and typeof cb isnt 'function'
            throw new err.ReqlDriverError "First argument to `toArray` must be a function or undefined."

        new Promise( (resolve, reject) =>
            toArrayCb = (err, result) ->
                if err?
                    reject(err)
                else
                    resolve(result)
            fn(toArrayCb)
        ).nodeify cb

    _makeEmitter: ->
        @emitter = new EventEmitter
        @each = ->
            throw new err.ReqlDriverError "You cannot use the cursor interface and the EventEmitter interface at the same time."
        @next = ->
            throw new err.ReqlDriverError "You cannot use the cursor interface and the EventEmitter interface at the same time."


    addListener: (event, listener) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.addListener(event, listener)

    on: (event, listener) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.on(event, listener)

    once: (event, listener) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.once(event, listener)

    removeListener: (event, listener) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.removeListener(event, listener)

    removeAllListeners: (event) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.removeAllListeners(event)

    setMaxListeners: (n) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.setMaxListeners(n)

    listeners: (event) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.listeners(event)

    emit: (args...) ->
        if not @emitter?
            @_makeEmitter()
            setImmediate => @_each @_eachCb
        @emitter.emit(args...)

    _eachCb: (err, data) =>
        if err?
            @emitter.emit('error', err)
        else
            @emitter.emit('data', data)

class Cursor extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    toString: ar () -> "[object Cursor]"

class Feed extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    hasNext: ->
        throw new err.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new err.ReqlDriverError "`toArray` is not available for feeds."

    toString: ar () -> "[object Feed]"

class UnionedFeed extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    hasNext: ->
        throw new err.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new err.ReqlDriverError "`toArray` is not available for feeds."

    toString: ar () -> "[object UnionedFeed]"

class AtomFeed extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    hasNext: ->
        throw new err.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new err.ReqlDriverError "`toArray` is not available for feeds."

    toString: ar () -> "[object AtomFeed]"

class OrderByLimitFeed extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    hasNext: ->
        throw new err.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new err.ReqlDriverError "`toArray` is not available for feeds."

    toString: ar () -> "[object OrderByLimitFeed]"

# Used to wrap array results so they support the same iterable result
# API as cursors.

class ArrayResult extends IterableResult
    # We store @__index as soon as the user starts using the cursor interface
    _hasNext: ar () ->
        if not @__index?
            @__index = 0
        @__index < @length

    _next: varar 0, 1, (cb) ->
        fn = (cb) =>
            if @_hasNext() is true
                self = @
                if self.__index%@stackSize is @stackSize-1
                    # Reset the stack
                    setImmediate ->
                        cb(null, self[self.__index++])
                else
                    cb(null, self[self.__index++])
            else
                cb new err.ReqlDriverError "No more rows in the cursor."

        new Promise( (resolve, reject) ->
            nextCb = (err, result) ->
                if (err)
                    reject(err)
                else
                    resolve(result)
            fn(nextCb)
        ).nodeify cb


    toArray: varar 0, 1, (cb) ->
        fn = (cb) =>
            # IterableResult.toArray would create a copy
            if @__index?
                cb(null, @.slice(@__index, @.length))
            else
                cb(null, @)

        new Promise( (resolve, reject) ->
            toArrayCb = (err, result) ->
                if (err)
                    reject(err)
                else
                    resolve(result)
            fn(toArrayCb)
        ).nodeify cb


    close: ->
        return @

    makeIterable: (response) ->
        response.__proto__ = {}
        for name, method of ArrayResult.prototype
            if name isnt 'constructor'
                if name is '_each'
                    response.__proto__['each'] = method
                    response.__proto__['_each'] = method
                else if name is '_next'
                    response.__proto__['next'] = method
                    response.__proto__['_next'] = method
                else
                    response.__proto__[name] = method

        response.__proto__.__proto__ = [].__proto__
        response

module.exports.Cursor = Cursor
module.exports.Feed = Feed
module.exports.AtomFeed = AtomFeed
module.exports.OrderByLimitFeed = OrderByLimitFeed
module.exports.makeIterable = ArrayResult::makeIterable
