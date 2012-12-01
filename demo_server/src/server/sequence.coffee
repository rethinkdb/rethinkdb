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

    union: (other) ->
        if (not @asArray?) or Object.prototype.toString.call(@asArray()) isnt '[object Array]'
            throw new RuntimeError "Required type: array but found #{typeof @asArray()}."
        if (not other.asArray?) or Object.prototype.toString.call(other.asArray()) isnt '[object Array]'
            throw new RuntimeError "Required type: array but found #{typeof other.asArray()}."

        new RDBArray @asArray().concat other.asArray()

    slice: (left, right) ->
        new RDBArray @asArray().slice left, right

    orderBy: (orderbys) ->
        new RDBArray @asArray().sort (a,b) ->
            for ob in orderbys
                if ob.asc
                    if not a[ob.attr]?
                        throw new RuntimeError "ORDERBY encountered a row missing attr '#{ob.attr}': #{DemoServer.prototype.convertToJSON(a)}"
                    if not b[ob.attr]?
                        throw new RuntimeError "ORDERBY encountered a row missing attr '#{ob.attr}': #{DemoServer.prototype.convertToJSON(b)}"
                    if a[ob.attr].gt(b[ob.attr]) then return true
                else
                    if not a[ob.attr]?
                        throw new RuntimeError "ORDERBY encountered a row missing attr '#{ob.attr}': #{DemoServer.prototype.convertToJSON(a)}"
                    if not b[ob.attr]?
                        throw new RuntimeError "ORDERBY encountered a row missing attr '#{ob.attr}': #{DemoServer.prototype.convertToJSON(b)}"
                    if a[ob.attr].lt(b[ob.attr]) then return true
            return false

    distinct: ->
        # Firefox's sort is unstable.
        sorted = @asArray().sort (a,b) ->
            if a.asJSON() < b.asJSON()
                return -1
            else if a.asJSON() > b.asJSON()
                return 1
            else
                return 0
        distinctd = [sorted[0]]
        for v in sorted[1..]
            unless (v.eq distinctd[distinctd.length-1])
                distinctd.push v
        return new RDBArray distinctd

    map: (mapping) ->
        result = @asArray().map (v) -> mapping(v)
        new RDBArray result

    reduce: (base, reduction) ->
        reduce_function = (acc, v) -> reduction(acc, v)
        @asArray().reduce reduce_function, base

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

    filter: (predicate) -> new RDBArray @asArray().filter (v) ->
        predicate_value = predicate(v).asJSON()
        if typeof predicate_value isnt 'boolean'
            throw new RuntimeError "Predicate failed to evaluate to a bool"
        return predicate_value

    between: (attr, lowerBound, upperBound) ->
        if typeof lowerBound.asJSON() isnt 'string' and typeof lowerBound.asJSON() isnt 'number'
            throw new RuntimeError "Lower bound of RANGE must be a string or a number, not #{DemoServer.prototype.convertToJSON(lowerBound.asJSON())}."
        if typeof upperBound.asJSON() isnt 'string' and typeof upperBound.asJSON() isnt 'number'
            throw new RuntimeError "Upper bound of RANGE must be a string or a number, not #{DemoServer.prototype.convertToJSON(upperBound.asJSON())}."

        result = []
        for v,i in @orderBy({attr:attr, asc:true}).asArray()
            if lowerBound.le(v[attr]) and upperBound.ge(v[attr])
                result.push(v)
        return new RDBArray result

    objSum = (arr, base) ->
        arr.forEach (val) ->
            for own k,v of base
                if k is 'first_error'
                    continue
                if val[k]?
                    base[k] += val[k]
            if (not base['first_error']?) and val['error']?
                base['first_error'] = val['error']
        base

    forEach: (mapping) ->
        results = @asArray().map (v) -> mapping(v)
        base = {inserted: 0, updated: 0}
        results.map (res) ->
            base = objSum res, base
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

    add: (other) ->
        if (not other.asArray?) or Object.prototype.toString.call(other.asArray()) isnt '[object Array]'
            throw new RuntimeError "Cannot ADD arrays to non-arrays"

        for value in other.asArray()
            @data.push value
        @
