goog.provide('rethinkdb.RDBSequence')

goog.require('rethinkdb.RDBJson')

class RDBSequence extends RDBJson
    asJSON: -> (v.asJSON() for v in @asArray())
    copy: -> new RDBArray (val.copy() for val in @asArray())
    eq: (other) ->
        self = @asArray()
        other = other.asArray()
        new RDBPrimitive (v.eq other[i] for v,i in self).reduce (a,b)->a&&b

    append: (val) ->
        new RDBArray @asArray().concat [val]

    asArray: ->
        throw new Error "Abstract method"

    count: -> new RDBPrimitive @asArray().length

    union: (other) ->
        new RDBArray @asArray().concat other.asArray()

    slice: (left, right) ->
        new RDBArray @asArray().slice left, right

    orderBy: (orderbys) ->
        new RDBArray @asArray().sort (a,b) ->
            for ob in orderbys
                if not (a[ob.attr]? or b[ob.attr]?)
                    obj = (if a[ob.attr]? then b else a)
                    throw new RuntimeError "ORDERBY encountered a row missing attr '#{ob.attr}': "+
                                           "#{Utils.stringify(obj.asJSON())}"
                op = (if ob.asc then 'gt' else 'lt')
                if a[ob.attr][op](b[ob.attr]).asJSON() then return true
            return false

    distinct: ->
        neu = []
        for v in @asArray()
            # The operator in doesn't work for objects
            foundCopy = false
            for w in neu
                if v.eq(w).asJSON() is true
                    foundCopy = true
                    break
            if foundCopy is false
                neu.push v
        new RDBArray neu

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
        result = []
        for v,i in @orderBy({attr:attr, asc:true}).asArray()
            if lowerBound.le(v[attr]).asJSON() and upperBound.ge(v[attr]).asJSON()
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
        base = {inserted: 0, errors: 0, updated: 0}
        results.map (res) ->
            base = objSum res, base
        base

    update: (mapping, demoServer) ->
        updated = 0
        skipped = 0
        errors = 0
        first_error = null
        @asArray().forEach (v) ->
            try
                v.update mapping
                updated++
            catch err
                unless first_error then first_error = demoServer.appendBacktrace err.message
                errors++
        result = {updated: updated, errors: errors, skipped: skipped}
        if first_error then result.first_error = first_error
        return result

    replace: (mapping, demoServer) ->
        modified = 0
        inserted = 0
        deleted = 0
        errors = 0
        first_error = null
        @asArray().forEach (v) ->
            try
                switch v.replace mapping
                    when "modified" then modified++
                    when "deleted"  then deleted++
                    when "inserted" then inserted++
            catch err
                unless first_error then first_error = demoServer.appendBacktrace err.message
                errors++

        result = {deleted: deleted, errors: errors, inserted: inserted, modified: modified}
        if first_error then result.first_error = first_error
        return result

    del: ->
        results = @asArray().map (v) -> v.del()
        objSum results, {deleted:0}

class RDBArray extends RDBSequence
    constructor: (arr) -> @data = arr
    buildFromJS: (jsArray) ->
        @data = []
        for value, index in jsArray
            if value is null
                @data.push new RDBPrimitive null
            else if typeof value is 'string' or typeof value is 'number' or typeof value is 'boolean'
                @data.push new RDBPrimitive value
            else if goog.isArray(value)
                arrayValue = new RDBArray
                arrayValue.buildFromJS value
                @data.push arrayValue
            else if typeof value is 'object'
                objectValue = new RDBObject
                objectValue.buildFromJS value
                @data.push objectValue

    asArray: -> @data

    add: (other) -> @union other

    eq: (other) ->
        if other.typeOf() isnt RDBJson.RDBTypes.ARRAY or
           @asArray().length isnt other.asArray().length
            return new RDBPrimitive false

        for v,i in @asArray()
            o = other.asArray()[i]
            if v.ne(o).asJSON() then return new RDBPrimitive false
        return new RDBPrimitive true

    le: (other) -> new RDBPrimitive (not @gt(other).asJSON())
    gt: (other) ->
        if other.typeOf() is RDBJson.RDBTypes.ARRAY
            for v,i in @asArray()
                o = other.asArray()[i]
                if not v.eq(o)
                    return v.gt(o)
            return new RDBPrimitive false
        return new RDBPrimitive @typeOf() > other.typeOf()

    ge: (other) -> new RDBPrimitive (not @lt(other).asJSON())
    lt: (other) ->
        if other.typeOf() is RDBJson.RDBTypes.ARRAY
            for v,i in @asArray()
                o = other.asArray()[i]
                if v.eq(o)
                    return v.lt(o)
            return new RDBPrimitive false
        return new RDBPrimitive @typeOf() < other.typeOf()
