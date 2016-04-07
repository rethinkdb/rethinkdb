error = require('./errors')
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
                        @_closeCb mkErr(error.ReqlServerCompileError, response, @_root)
                    when protoResponseType.CLIENT_ERROR
                        @_closeCb mkErr(error.ReqlClientError, response, @_root)
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
        if @_closeCbPromise?
            cb = @_getCallback()
            cb new error.ReqlDriverError "Cursor is closed."
        # If there are no more waiting callbacks, just wait until the next event
        while @_cbQueue[0]?
            if @bufferEmpty() is true
                # We prefetch things here, set `is 0` to avoid prefectch
                if @_endFlag is true
                    cb = @_getCallback()
                    cb new error.ReqlDriverError "No more rows in the cursor."
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
                        cb mkErr(error.ReqlServerCompileError, response, @_root)
                    when protoResponseType.CLIENT_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        cb mkErr(error.ReqlClientError, response, @_root)
                    when protoResponseType.RUNTIME_ERROR
                        @_responses.shift()
                        cb = @_getCallback()
                        errType = util.errorClass(response.e)
                        cb mkErr(errType, response, @_root)
                    else
                        @_responses.shift()
                        cb = @_getCallback()
                        cb new error.ReqlDriverError "Unknown response type for cursor"

    _promptCont: ->
        # Let's ask the server for more data if we haven't already
        if (not @_contFlag) and (not @_endFlag) and @_conn.isOpen()
            @_contFlag = true
            @_outstandingRequests += 1
            @_conn._continueQuery(@_token)


    ## Implement IterableResult
    hasNext: ->
        throw new error.ReqlDriverError "The `hasNext` command has been removed since 1.13. Use `next` instead."

    _next: varar 0, 1, (cb) ->
        if cb? and typeof cb isnt "function"
            throw new error.ReqlDriverError "First argument to `next` must be a function or undefined."

        fn = (cb) =>
            @_cbQueue.push cb
            @_promptNext()

        return Promise.fromNode(fn).nodeify(cb)


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
                # Also clear any buffered results, so future calls to
                # `next` fail.
                @_responses = []
                @_responseIndex = 0
            else
                # We aren't ended, and we need to. Create a promise
                # that's resolved when the END query is acknowledged.
                @_closeCbPromise = new Promise((resolve, reject) =>
                    @_closeCb = (err) =>
                        # Clear any buffered results, so future calls to
                        # `next` fail.
                        @_responses = []
                        @_responseIndex = 0
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

    each: varar(1, 2, (cb, onFinished) ->
        unless typeof cb is 'function'
            throw new error.ReqlDriverError "First argument to each must be a function."
        if onFinished? and typeof onFinished isnt 'function'
            throw new error.ReqlDriverError "Optional second argument to each must be a function."

        self = @
        nextCb = (err, data) =>
            if err?
                if err.message is 'No more rows in the cursor.'
                    onFinished?()
                else
                    cb(err)
            else if cb(null, data) isnt false
                @_next nextCb
            else
                onFinished?()
        @_next nextCb
    )

    eachAsync: varar(1, 3, (cb, errCb, options = { concurrency: 1 }) ->
        unless typeof cb is 'function'
            throw new error.ReqlDriverError 'First argument to eachAsync must be a function.'

        if errCb?
            if typeof errCb is 'object'
                options = errCb
                errCb = undefined
            else if typeof errCb isnt 'function'
                throw new error.ReqlDriverError "Optional second argument to eachAsync must be a function or `options` object"

        unless options and typeof options.concurrency is 'number' and options.concurrency > 0
            throw new error.ReqlDriverError "Optional `options.concurrency` argument to eachAsync must be a positive number"

        pending = []

        userCb = (data) ->
            if cb.length <= 1
                ret = Promise.resolve(cb(data)) # either synchronous or awaits promise
            else
                handlerCalled = false
                doneChecking = false
                handlerArg = undefined
                ret = Promise.fromNode (handler) ->
                    asyncRet = cb(data, (err) ->
                        handlerCalled = true
                        if doneChecking
                            handler(err)
                        else
                            handlerArg = err
                        ) # callback-style async
                    unless asyncRet is undefined
                        handler(new error.ReqlDriverError "A two-argument row handler for eachAsync may only return undefined.")
                    else if handlerCalled
                        handler(handlerArg)
                    doneChecking = true
            return ret
            .then (data) ->
                return data if data is undefined or typeof data is Promise
                throw new error.ReqlDriverError "Row handler for eachAsync may only return a Promise or undefined."
        nextCb = =>
            if @_closeCbPromise?
                return Promise.resolve().then (data) ->
                    throw new error.ReqlDriverError "Cursor is closed."
            else
                return @_next().then (data) ->
                    return data if pending.length < options.concurrency
                    return Promise.any(pending)
                    .catch Promise.AggregateError, (errs) -> throw errs[0]
                    .return(data)
                .then (data) ->
                    p = userCb(data).then ->
                        pending.splice pending.indexOf(p), 1
                    pending.push p
                .then nextCb
                .catch (err) ->
                    throw err if err?.message isnt 'No more rows in the cursor.'
                    return Promise.all(pending) # await any queued promises before returning

        resPromise = nextCb().then () ->
            errCb(null) if errCb?
        .catch (err) ->
            return errCb(err) if errCb?
            throw err
        return resPromise unless errCb?
        return null
    )

    _each: @::each
    _eachAsync: @::eachAsync

    toArray: varar 0, 1, (cb) ->
        if cb? and typeof cb isnt 'function'
            throw new error.ReqlDriverCompileError "First argument to `toArray` must be a function or undefined."

        results = []
        wrapper = (res) =>
            results.push(res)
            return undefined
        return @eachAsync(wrapper).then(() =>
            return results
        ).nodeify(cb)

    _makeEmitter: ->
        @emitter = new EventEmitter
        @each = ->
            throw new error.ReqlDriverError "You cannot use the cursor interface and the EventEmitter interface at the same time."
        @next = ->
            throw new error.ReqlDriverError "You cannot use the cursor interface and the EventEmitter interface at the same time."


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
        throw new error.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new error.ReqlDriverError "`toArray` is not available for feeds."

    toString: ar () -> "[object Feed]"

class UnionedFeed extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    hasNext: ->
        throw new error.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new error.ReqlDriverError "`toArray` is not available for feeds."

    toString: ar () -> "[object UnionedFeed]"

class AtomFeed extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    hasNext: ->
        throw new error.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new error.ReqlDriverError "`toArray` is not available for feeds."

    toString: ar () -> "[object AtomFeed]"

class OrderByLimitFeed extends IterableResult
    constructor: ->
        @_type = protoResponseType.SUCCESS_PARTIAL
        super

    hasNext: ->
        throw new error.ReqlDriverError "`hasNext` is not available for feeds."
    toArray: ->
        throw new error.ReqlDriverError "`toArray` is not available for feeds."

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
            if @_closeCbPromise?
                cb new error.ReqlDriverError "Cursor is closed."
            if @_hasNext() is true
                self = @
                if self.__index%@stackSize is @stackSize-1
                    # Reset the stack
                    setImmediate ->
                        cb(null, self[self.__index++])
                else
                    cb(null, self[self.__index++])
            else
                cb new error.ReqlDriverError "No more rows in the cursor."

        return Promise.fromNode(fn).nodeify(cb)

    toArray: varar 0, 1, (cb) ->
        fn = (cb) =>
            if @_closeCbPromise?
                cb(new error.ReqlDriverError("Cursor is closed."))

            # IterableResult.toArray would create a copy
            if @__index?
                cb(null, @.slice(@__index, @.length))
            else
                cb(null, @)

        return Promise.fromNode(fn).nodeify(cb)

    close: varar 0, 1, (cb) ->
        # Clear the array
        @.length = 0
        @__index = 0
        # We set @_closeCbPromise so that functions such as `eachAsync`
        # know that we have been closed and can error accordingly.
        @_closeCbPromise = Promise.resolve().nodeify(cb)
        return @_closeCbPromise

    makeIterable: (response) ->
        response.__proto__ = {}
        for name, method of ArrayResult.prototype
            if name isnt 'constructor'
                if name is '_next'
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
