goog.provide("rethinkdb.cursor")

goog.require("rethinkdb.base")

class Cursor
    constructor: (rootTerm, cb) ->
        @root = rootTerm # Saved for backtrace printing
        @cb = cb
        @iter = null
        @iterPassed = false
    
    goResult: (result) ->
        @cb null, result

    goError: (err) ->
        @cb err, undefined

    endIter: (results) ->
        unless @iter?
            @iter = new LazyIterator
        @iter._endResults results
        unless @iterPassed
            @cb null, @iter
            @iterPassed = true

    partIter: (results) ->
        unless @iter?
            @iter = new LazyIterator
        @iter._addResults results
        unless @iterPassed
            @cb null, @iter
            @iterPassed = true

class LazyIterator
    constructor: ->
        @_results = []
        @_index = 0
        @_endFlag = false
        @_cont = null
        
    _addResults: (results) ->
        @_results = @_results.concat results
        @_prompt()

    _endResults: (results) ->
        @_endFlag = true
        @_addResults results

    _prompt: ->
        if @_cont?
            @_cont()

    more: -> @_endFlag && @_index >= @_results.length

    forEach: (cb) ->
        @_cont = ->
            while @_index < @_results.length
                cb @_results[@_index++]
        @_prompt()

    collect: (cb) ->
        @_cont = ->
            if @_endFlag
                cb @_results
        @_prompt()

    toString: ->
        if @_endFlag and @_results.length < 8
            "[#{@_results}]"
        else
            "[#{@_results[..8]}, ...]"
