goog.provide('rethinkdb.server.RDBJson')

class RDBJson
    abstract = new Error "Abstract method"

    asJSON: -> throw abstract
    copy: -> throw abstract

    not: -> throw abstract

    eq: (other) -> throw abstract
    ne: (other) -> throw abstract
    lt: (other) -> throw abstract
    le: (other) -> throw abstract
    gt: (other) -> throw abstract
    ge: (other) -> throw abstract

    add: (other) -> throw abstract
    sub: (other) -> throw abstract
    mul: (other) -> throw abstract
    div: (other) -> throw abstract
    mod: (other) -> throw abstract

class RDBPrimitive
    constructor: (val) -> @data = val
    asJSON: -> @data
    copy: -> new RDBPrimitive @data

    not: -> not @data

    eq: (other) -> @data is other.asJSON()
    ne: (other) -> not @eq other
    lt: (other) -> @data <  other.asJSON()
    le: (other) -> @data <= other.asJSON()
    gt: (other) -> @data >  other.asJSON()
    ge: (other) -> @data >= other.asJSON()

    add: (other) -> @data + other.asJSON()
    sub: (other) -> @data - other.asJSON()
    mul: (other) -> @data * other.asJSON()
    div: (other) -> @data / other.asJSON()
    mod: (other) -> @data % other.asJSON()


class RDBObject extends RDBJson
    asJSON: ->
        obj = {}
        for own k,v of @
            obj[k] = v.asJSON()
        return obj

    # Needed to prevent modification of table rows
    copy: ->
        result = new RDBObject
        for own k,v of @
            result[k] = v.copy()
        return result

    eq: (other) ->
        ((other[k]? && v.eq other[k]) for own k,v of @).reduce (a,b) -> a&&b

    merge: (other) ->
        self = @copy()
        for own k,v of other
            self[k] = v
        return self

    pick: (attrs) ->
        self = @copy()
        result = new RDBObject
        for k in attrs
            result[k] = self[k]
        return result

    unpick: (attrs) ->
        self = @copy()
        for k in attrs
            delete self[k]
        return self
