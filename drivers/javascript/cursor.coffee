err = require('./errors')
util = require('./util')
pb = require('./protobuf')

# Import some names to this namespace for convienience
ar = util.ar
varar = util.varar
aropt = util.aropt

if not setImmediate?
    setImmediate = (cb) ->
        setTimeout cb, 0

deconstructDatum = (datum, opts) ->
    pb.DatumTypeSwitch(datum, {
        "R_JSON": =>
            JSON.parse(datum.r_str)
       ,"R_NULL": =>
            null
       ,"R_BOOL": =>
            datum.r_bool
       ,"R_NUM": =>
            datum.r_num
       ,"R_STR": =>
            datum.r_str
       ,"R_ARRAY": =>
            deconstructDatum(dt, opts) for dt in datum.r_array
       ,"R_OBJECT": =>
            obj = {}
            for pair in datum.r_object
                obj[pair.key] = deconstructDatum(pair.val, opts)

            # An R_OBJECT may be a regular object or a "pseudo-type" so we need a
            # second layer of type switching here on the obfuscated field "$reql_type$"
            switch obj['$reql_type$']
                when 'TIME'
                    switch opts.timeFormat
                        # Default is native
                        when 'native', undefined
                            if not obj['epoch_time']?
                                throw new err.RqlDriverError "pseudo-type TIME #{obj} object missing expected field 'epoch_time'."

                            # We ignore the timezone field of the pseudo-type TIME object. JS dates do not support timezones.
                            # By converting to a native date object we are intentionally throwing out timezone information.

                            # field "epoch_time" is in seconds but the Date constructor expects milliseconds
                            (new Date(obj['epoch_time']*1000))
                        when 'raw'
                            # Just return the raw (`{'$reql_type$'...}`) object
                            obj
                        else
                            throw new err.RqlDriverError "Unknown timeFormat run option #{opts.timeFormat}."
                else
                    # Regular object or unknown pseudo type
                    obj
        },
            => throw new err.RqlDriverError "Unknown Datum type"
        )

# setImmediate is not defined in some browsers (including Chrome)

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
    constructor: (conn, token) ->
        @_conn = conn
        @_token = token

        @_chunks = []
        @_endFlag = false
        @_contFlag = false
        @_cont = null
        @_cbQueue = []

    _addChunk: (chunk) ->
        if chunk.length > 0
            @_chunks.push chunk

    _addData: (chunk) ->
        @_addChunk chunk

        if @_chunks.length == 1
            @_promptCont()

        @_contFlag = false
        @_promptNext()
        @

    _endData: (chunk) ->
        @_addChunk chunk
        @_endFlag = true

        @_contFlag = true
        @_promptNext()
        @

    _promptNext: ->

        # If there are no more waiting callbacks, just wait until the next event
        while @_cbQueue[0]?

            # If there's no more data let's notify the waiting callback
            if not @hasNext()
                cb = @_cbQueue.shift()
                cb new err.RqlDriverError "No more rows in the cursor."
            else

                # We haven't processed all the data, let's try to give it to the callback

                # Is there data waiting in our buffer?
                chunk = @_chunks[0]
                if @_chunks.length == 1
                    # We're out of data for now, let's fetch more (which will prompt us again)
                    @_promptCont()
                
                    if chunk.length == 1 && !@_endFlag
                        return

                # After this point there is at least one row in the chunk

                row = chunk.shift()
                cb = @_cbQueue.shift()

                # Did we just empty the chunk?
                if chunk[0] is undefined
                    # We're done with this chunk, discard it
                    @_chunks.shift()

                # Finally we can invoke the callback with the row
                cb null, row

    _promptCont: ->
        # Let's ask the server for more data if we haven't already
        unless @_contFlag
            @_conn._continueQuery(@_token)
            @_contFlag = true


    ## Implement IterableResult

    hasNext: ar () -> @_chunks[0]?

    next: ar (cb) ->
        nextCbCheck(cb)
        @_cbQueue.push cb
        @_promptNext()

    close: ar () ->
        unless @_endFlag
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

    makeIterable: (response) ->
        for name, method of ArrayResult.prototype
            if name isnt 'constructor'
                response.__proto__[name] = method
        response

nextCbCheck = (cb) ->
    unless typeof cb is 'function'
        throw new err.RqlDriverError "Argument to next must be a function."

module.exports.deconstructDatum = deconstructDatum
module.exports.Cursor = Cursor
module.exports.makeIterable = ArrayResult::makeIterable
