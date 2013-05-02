goog.provide("rethinkdb.cursor")

goog.require("rethinkdb.base")

class IterableResult
    hasNext: -> throw "Abstract Method"
    next: -> throw "Abstract Method"

    each: ar (cb) ->
        n = =>
            @next (err, row) =>
                cb(err, row)
                if @hasNext()
                    n()
        if @hasNext()
            n()

    toArray: ar (cb) ->
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
                cb new RqlDriverError "No more rows in the cursor."
            else

                # We haven't processed all the data, let's try to give it to the callback

                # Is there data waiting in our buffer?
                chunk = @_chunks[0]
                if not chunk?
                    # We're out of data for now, let's fetch more (which will prompt us again)
                    @_promptCont()
                    return
                else

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

    hasNext: ar () -> !@_endFlag || @_chunks[0]?

    next: ar (cb) ->
        @_cbQueue.push cb
        @_promptNext()

    close: ar () ->
        unless @_endFlag
            @_conn._end(@_token)

    toString: ar () -> "[object Cursor]"

# Used to wrap array results so they support the same iterable result
# API as cursors.
class ArrayResult extends IterableResult
    hasNext: ar () -> (@__index < @.length)
    next: ar (cb) -> cb(null, @[@__proto__.__index++])

    makeIterable: (response) ->
        for name, method of ArrayResult.prototype
            response.__proto__[name] = method
        response.__proto__.__index = 0
        response

