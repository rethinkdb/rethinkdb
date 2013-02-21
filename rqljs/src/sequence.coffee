goog.provide('rethinkdb.RDBSequence')

goog.require('rethinkdb.RDBType')

class RDBSequence extends RDBType
    asJSON: -> (v.asJSON() for v in @asArray())
    copy: -> new RDBArray (val.copy() for val in @asArray())
    eq: (other) ->
        self = @asArray()
        other = other.asArray()
        new RDBPrimitive (v.eq other[i] for v,i in self).reduce (a,b)->a&&b

    nth: (index) ->
        i = index.asJSON()
        if i < 0 then throw new RqlRuntimeError "Nth doesn't support negative indicies"
        if i >= @asArray().length then throw new RqlRuntimeError "Index too large"
        @asArray()[index.asJSON()]

    append: (val) -> new RDBArray @asArray().concat [val]
    asArray: -> throw new ServerError "Abstract method"
    count: -> new RDBPrimitive @asArray().length
    union: (others...) -> new RDBArray @asArray().concat (others.map (v)->v.asArray())...
    slice: (left, right) -> new RDBArray @asArray()[left.asJSON()..right.asJSON()]
    limit: (right) ->
        num = right.asJSON()
        if num < 0 then num = 0
        new RDBArray @asArray()[0...num]

    pluck: (attrs...) -> @map (row) -> row.pluck(attrs...)
    without: (attrs...) -> @map (row) -> row.without(attrs...)

    orderBy: (orderbys) ->
        new RDBArray @asArray().sort (a,b) ->
            for ob in orderbys.asArray()
                ob = ob.asJSON()
                if ob[0] == '-'
                    op = 'lt'
                    ob = ob.slice(1)
                else
                    op = 'gt'
                if a[ob][op](b[ob]).asJSON() then return 1
            return -1

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
            groupID = groupMapping(doc)
            actualGroupID = groupID.asJSON()
            unless groups[actualGroupID]?
                groups[actualGroupID] = []
                groups[actualGroupID]._groupID = groupID
            groups[actualGroupID].push doc

        new RDBArray (for own groupID,group of groups
            new RDBObject {
                'group': group._groupID
                'reduction': (new RDBArray group).map(valueMapping).reduce(reduction)
            }
        )

    dataCollectors =
        COUNT: ->
            mapping: (row) -> new RDBPrimitive 1
            reduction: (a,b) -> a.add(b)
            finalizer: (v) -> v
        SUM: (attr) ->
            mapping: (row) -> row[attr.asJSON()]
            reduction: (a,b) -> a.add(b)
            finalizer: (v) -> v
        AVG: (attr) ->
            mapping: (row) -> new RDBObject {'count': 1, 'sum': row[attr.asJSON()]}
            reduction: (a,b) -> new RDBObject {'count':a['count'].add(b['count']), 'sum':a['sum'].add(b['sum'])}
            finalizer: (v) -> v['sum'].div(v['count'])

    groupBy: (fields, aggregator) ->
        for own col,arg of aggregator
            collector = dataCollectors[col]
            unless collector?
                throw new RqlRuntimeError "No such aggregator as #{col}"
            collector = collector(arg)
            break

        @groupedMapReduce((row) ->
            fields.map (f) -> row[f.asJSON()]
        , collector.mapping
        , collector.reduction
        ).map (group) -> group.merge({'reduction': collector.finalizer(group['reduction'])})

    concatMap: (mapping) ->
        new RDBArray Array::concat.apply [], @map(mapping).map((v)->v.asArray()).asArray()

    filter: (predicate) -> new RDBArray @asArray().filter (v) -> predicate(v).asJSON()

    between: (lowerBound, upperBound) ->
        if @asArray().length == 0
            # Return immediately because getPK does not work on empty arrays
            return new RDBArray []
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
    eqJoin: (left_attr, right) ->
        if right.asArray().length == 0
            # Return immediately because getPK does not work on empty arrays
            return new RDBArray []
        right_attr = right.getPK()
        @innerJoin right, (lRow, rRow) ->
            lRow[left_attr.asJSON()].eq(rRow[right_attr.asJSON()])

    zip: -> @map (row) -> row['left'].merge(row['right'])

    statsMerge = (results) ->
        base = new RDBObject {}
        for result in results.asArray()
            for own k,v of result
                if base[k]?
                    switch v.typeOf()
                        when RDBType.NUMBER
                            base[k] = base[k].add(v)
                        when RDBType.STRING
                            null # left preferential
                        when RDBType.ARRAY
                            base[k] = base[k].union(v)
                else
                    base[k] = v
        return base

    forEach: (mapping) -> statsMerge @map mapping

    getPK: -> @asArray()[0].getPK()

    update: (mapping) -> statsMerge @map (row) -> row.update mapping
    replace: (mapping) -> statsMerge @map (row) -> row.replace mapping
    del: -> statsMerge @map (v) -> v.del()

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
