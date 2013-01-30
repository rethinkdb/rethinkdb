goog.provide('rethinkdb.RDBSequence')

goog.require('rethinkdb.RDBType')

class RDBSequence extends RDBType
    asJSON: -> (v.asJSON() for v in @asArray())
    copy: -> new RDBArray (val.copy() for val in @asArray())
    eq: (other) ->
        self = @asArray()
        other = other.asArray()
        new RDBPrimitive (v.eq other[i] for v,i in self).reduce (a,b)->a&&b

    nth: (index) -> @asArray()[index.asJSON()]
    append: (val) -> new RDBArray @asArray().concat [val]
    asArray: -> throw new ServerError "Abstract method"
    count: -> new RDBPrimitive @asArray().length
    union: (others...) -> new RDBArray @asArray().concat (others.map (v)->v.asArray())...
    slice: (left, right) -> new RDBArray @asArray()[left.asJSON()..right.asJSON()]

    orderBy: (orderbys) ->
        new RDBArray @asArray().sort (a,b) ->
            for ob in orderbys.asArray()
                op = (if ob.asJSON()[0] == '-' then 'lt' else 'gt')
                if a[ob.asJSON()][op](b[ob.asJSON()]).asJSON() then return true
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

    map: (mapping) -> new RDBArray @asArray().map mapping
    reduce: (reduction, base) ->
        # This is necessary because of the strange behavior of builtin reduce. It seems to
        # distinguish between the no second argument case and the `undefined` passed as the
        # second argument case, passing undefined to the first call to the reduction function
        # in the latter case. In a user land reduce implementation that would not be possible.
        if base is undefined
            @asArray().reduce reduction
        else
            @asArray().reduce reduction, base

    groupedMapReduce: (groupMapping, valueMapping, reduction) ->
        groups = {}
        @asArray().forEach (doc) ->
            groupID = groupMapping(doc).asJSON()
            unless groups[groupID]?
                groups[groupID] = []
                groups[groupID]._actualGroupID = groupID
            groups[groupID].push doc

        new RDBArray (for own groupID,group of groups
            new RDBObject {
                'group': new RDBPrimitive group._actualGroupID
                'reduction': (new RDBArray group).map(valueMapping).reduce(reduction)
            }
        )

    aggregator:
        count:
            mapping: (row) -> new RDBPrimitive 1
            reduction: (a,b) -> a.add(b)
            finalizer: (row) -> row

    groupBy: (fields, aggregator) ->
        agg = aggregators[aggregator]
        unless agg? then throw RuntimeError "No such aggregator"
        @groupedMapReduce((row) ->
            row[fields.asArray()[0].asJSON()]
        , agg.mapping
        , agg.reduction
        ).map (group) ->
            group.merge({'reduction': agg.finalizer(group['reduction'])})

    concatMap: (mapping) ->
        new RDBArray Array::concat.apply [], @map(mapping).map((v)->v.asArray()).asArray()

    filter: (predicate) -> new RDBArray @asArray().filter (v) -> predicate(v).asJSON()

    between: (lowerBound, upperBound) ->
        attr = @getPK()
        result = []
        for v,i in @orderBy(new RDBArray [@getPK()]).asArray()
            if (lowerBound is undefined || lowerBound.le(v[attr.asJSON()]).asJSON()) and
               (upperBound is undefined || upperBound.ge(v[attr.asJSON()]).asJSON())
                result.push(v)
        return new RDBArray result

    innerJoin: (right, predicate) ->
        @concatMap (lRow) ->
            right.concatMap (rRow) ->
                if predicate(lRow, rRow).asJSON()
                    new RDBArray [new RDBObject {left: lRow, right: rRow}]
                else
                    new RDBArray []

    outerJoin: (right, predicate) ->
        @concatMap (lRow) ->
            forInL = right.concatMap (rRow) ->
                if predicate(lRow, rRow).asJSON()
                    new RDBArray [new RDBObject {left: lRow, right: rRow}]
                else
                    new RDBArray []
            if forInL.count().asJSON() > 0
                return forInL
            else
                return new RDBArray [new RDBObject {left: lRow}]

    # We're just going to implement this on top of inner join
    eqJoin: (right, {left_attr, right_attr}) ->
        unless left_attr
            left_attr = @getPK().asJSON()
        unless right_attr
            right_attr = right.getPK().asJSON()
        @innerJoin right, (lRow, rRow) ->
            lRow[left_attr.asJSON()].eq(rRow[right_attr.asJSON()])

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
        results = @asArray().map mapping
        base = {inserted: 0, errors: 0, updated: 0}

        #TODO results is empty. Why are the write results not propogating?

        #results.map (res) ->
        #    base = objSum res, base
        new RDBObject base

    getPK: -> @asArray()[0].getPK()

    update: (mapping) ->
        updated = 0
        skipped = 0
        errors = 0
        first_error = null
        @asArray().forEach (v) ->
            try
                v.update mapping
                updated++
            catch err
                console.log err
                unless first_error then first_error = err
                errors++
        result = {updated: updated, errors: errors, skipped: skipped}
        if first_error then result.first_error = first_error
        return new RDBObject result

    replace: (mapping) ->
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
                unless first_error then first_error = err
                errors++

        result = {deleted: deleted, errors: errors, inserted: inserted, modified: modified}
        if first_error then result.first_error = first_error
        return new RDBObject result

    del: ->
        results = @asArray().map (v) -> v.del()
        new RDBObject objSum results, {deleted:0}

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
