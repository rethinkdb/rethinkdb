goog.provide('rethinkdb.server.RDBSequence')

goog.require('rethinkdb.server.RDBJson')

class RDBSequence extends RDBJson
    asJSON: -> (v.asJSON() for v in @asArray())
    copy: -> new RDBArray (val.copy() for val in @asArray())
    eq: (other) ->
        self = @asArray()
        other = other.asArray()
        (v.eq other[i] for v,i in self).reduce (a,b)->a&&b

    append: (val) ->
        new RDBArray @asArray().concat [val]

    asArray: ->
        throw new Error "Abstract method"

    orderBy: (k) ->
        new RDBArray @asArray().sort (a,b) -> a[k] < b[k]

    map: (mapping) ->
        new RDBArray @asArray().map (v) -> mapping(v)

    concatMap: (mapping) ->
        new RDBArray Array::concat.apply [], @asArray().map((v) -> mapping(v).asArray())

    filter: (predicate) ->
        new RDBArray @asArray().filter (v) -> predicate(v).asJSON()

    between: -> throw new RuntimeError "Not implemented"

class RDBArray extends RDBSequence
    constructor: (arr) -> @data = arr
    asArray: -> @data
