goog.provide('rethinkdb.RDBSequence')

goog.require('rethinkdb.RDBType')

class RDBSequence extends RDBType
    asJSON: -> (v.asJSON() for v in @asArray())
    copy: -> new RDBArray (val.copy() for val in @asArray())
    eq: (other) ->
        self = @asArray()
        other = other.asArray()
        new RDBPrimitive (v.eq other[i] for v,i in self).reduce (a,b)->a&&b

    append: (val) -> new RDBArray @asArray().concat [val]
    asArray: -> throw new ServerError "Abstract method"
    count: -> new RDBPrimitive @asArray().length
    union: (other) -> new RDBArray @asArray().concat other.asArray()
    slice: (left, right) -> new RDBArray @asArray().slice left, right

    orderBy: (orderbys) ->
        new RDBArray @asArray().sort (a,b) ->
            for ob in orderbys
                op = (if ob.asc then 'gt' else 'lt')
                if a[ob.attr][op](b[ob.attr]).asJSON() then return new RDBPrimitive true
            return new RDBPrimitive false

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

    map: (mapping) -> new RDBArray @asArray().map mapping
    reduce: (reduction, base) -> new RDBArray @asArray().reduce reduction, base

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
        new RDBArray Array::concat.apply [], @map(mapping).asArray()

    filter: (predicate) -> new RDBArray @asArray().filter (v) -> predicate(v).asJSON()

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
    asArray: -> @data

    add: (other) -> @union other

    eq: (other) ->
        unless @count().eq(other.count()).asJSON() then return new RDBPrimitive false
        (return new RDBPrimitive false for v,i in @asArray() when v isnt other.asArray()[i])

    lt: (other) ->
        for v,i in @asArray()
            if v.lt(other.asArray()[i]).asJSON() then return new RDBPrimitive false
        return other.count().ge(@count)
