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
        new RDBArray @asArray().sort (b,a) -> a[k].lt(b[k])

    distinct: ->
        sorted = @asArray().sort (a,b) -> a < b
        distinctd = [sorted[0]]
        for v in sorted[1..]
            unless (v.eq distinctd[distinctd.length-1])
                distinctd.push v
        return new RDBArray distinctd

    map: (mapping) ->
        new RDBArray @asArray().map (v) -> mapping(v)

    reduce: (base, reduction) ->
        @asArray().reduce ((acc, v) -> reduction(acc, v)), base

    groupedMapReduce: (groupMapping, valueMapping, reduction) ->
        groups = {}
        @asArray().forEach (doc) ->
            groupID = (groupMapping doc).asJSON()
            unless groups[groupID]?
                groups[groupID] = []
                groups[groupID]._actualGroupID = groupID
            groups[groupID].push doc

        new RDBArray (for own groupID,group of groups
            res = new RDBObject
            res['group'] = new RDBPrimitive group._actualGroupID
            res['reduction'] = (group.map valueMapping).reduce reduction
            res
        )

    concatMap: (mapping) ->
        new RDBArray Array::concat.apply [], @asArray().map((v) -> mapping(v).asArray())

    filter: (predicate) -> new RDBArray @asArray().filter (v) -> predicate(v).asJSON()

    between: (attr, lowerBound, upperBound) ->
        result = []
        for v,i in @orderBy(attr).asArray()
            if lowerBound.le(v[attr]) and upperBound.ge(v[attr])
                result.push(v)
        return new RDBArray result

    objSum = (arr, base) ->
        arr.forEach (val) ->
            for own k,v of base
                if val[k]?
                    base[k] += val[k]
        base

    update: (mapping) ->
        results = @asArray().map (v) -> v.update mapping
        objSum results, {updated: 0, errors: 0, skipped: 0}

    replace: (mapping) ->
        results = @asArray().map (v) -> v.replace mapping
        objSum results, {deleted:0, errors:0, inserted:0, modified:0}

    del: ->
        results = @asArray().map (v) -> v.del()
        objSum results, {deleted:0}

class RDBArray extends RDBSequence
    constructor: (arr) -> @data = arr
    asArray: -> @data
