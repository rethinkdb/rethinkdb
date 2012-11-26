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

    count: -> new RDBPrimitive @asArray().length

    union: (other) -> new RDBArray @asArray().concat other.asArray()

    slice: (left, right) ->
        new RDBArray @asArray().slice left, right

    orderBy: (k) ->
        new RDBArray @asArray().sort (a,b) -> a[k].lt(b[k])

    distinct: ->
        sorted = @asArray().sort (a,b) -> a < b
        distinctd = [sorted[0]]
        for v in sorted[1..]
            unless (v.eq distinctd[distinctd.length-1])
                distinctd.push v
        return new RDBArray distinctd

    map: (mapping) ->
        new RDBArray @asArray().map (v) -> mapping(v)

    concatMap: (mapping) ->
        new RDBArray Array::concat.apply [], @asArray().map((v) -> mapping(v).asArray())

    filter: (predicate) -> new RDBArray @asArray().filter (v) -> predicate(v).asJSON()

    between: (attr, lowerBound, upperBound) ->
        result = []
        for v,i in @orderBy(attr)
            if lowerBound.le(v) and upperBound.ge(v)
                result.push(v)
        return new RDBArray result

    update: (mapping) -> @asArray().map (v) -> v.update mapping
    replace: (mapping) -> @asArray().map (v) -> v.replace mapping
    del: -> @asArray().map (v) -> v.del

class RDBArray extends RDBSequence
    constructor: (arr) -> @data = arr
    asArray: -> @data
