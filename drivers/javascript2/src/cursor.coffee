goog.provide("rethinkdb.cursor")

goog.require("rethinkdb.base")

class Cursor
    constructor: (conn, token) ->
        @_conn = conn
        @_token = token
        @_data = []
        @_index = 0
        @_endFlag = false
        @_contFlag = false
        @_cont = null
        
    _addData: (data) =>
        @_data = @_data.concat data
        @_contFlag = false
        @_prompt()
        @

    _endData: (data) =>
        @_endFlag = true
        @_addData data
        @_contFlag = true
        @

    _prompt: =>
        if @_cont?
            @_cont()

    _getMore: =>
        unless @_contFlag
            @_conn._continueQuery(@_token)
            @_contFlag = true

    hasNext: => !@_endFlag || @_index < @_data.length

    next: (cb) =>
        @_cont = =>
            if @_index < @_data.length
                cb null, @_data[@_index++]
            else
                @_conn.outstandingCallbacks[@_token]['cursor'] = @
                @_getMore()
        @_prompt()

    each: (cb) =>
        @_cont = =>
            while @_index < @_data.length
                cb null, @_data[@_index++]
            @_conn.outstandingCallbacks[@_token]['cursor'] = @
            @_getMore() # TODO: We should save cb and fire cb instead of the callback in outstandingCallbacks - Let's ask Bill
        @_prompt()

    toArray: (cb) =>
        @_cont = =>
            if @_endFlag
                cb null, @_data
            else
                @_conn.outstandingCallbacks[@_token]['cursor'] = @
                @_getMore()
        @_prompt()

    close: =>
        unless @_endFlag
            @_conn._end(@_token)

    toString: ->
        if @_endFlag and @_data.length < 8
            "[#{@_data}]"
        else
            "[#{@_data[..8]}, ...]"
