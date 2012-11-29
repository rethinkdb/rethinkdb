goog.provide('rethinkdb.server.RDBJson')

class RDBJson
    abstract = new Error "Abstract method"

    isNull: -> (@ instanceof RDBPrimitive and @asJSON() is null)

    asJSON: -> throw abstract
    copy: -> throw abstract

    not: -> throw abstract

    eq: (other) -> throw abstract
    ne: (other) -> not @eq other
    lt: (other) -> throw abstract
    le: (other) -> throw abstract
    gt: (other) -> throw abstract
    ge: (other) -> throw abstract

    add: (other) -> throw abstract
    sub: (other) -> throw abstract
    mul: (other) -> throw abstract
    div: (other) -> throw abstract
    mod: (other) -> throw abstract

# Kind of a psudo class, it only supports one static method that mixes in selection functionality
class RDBSelection
    makeSelection: (obj, table) ->
        proto =
            update: (mapping) ->
                try
                    update_doc = mapping @
                catch err
                    if err instanceof MissingAttribute
                        return {errors: 1, error: "Object:\n#{DemoServer.prototype.convertToJSON(err.object)}\nis missing attribute #{DemoServer.prototype.convertToJSON(err.attribute)}"}
                    else # Can it happen?
                        throw err
                if update_doc[table.primaryKey]? and update_doc[table.primaryKey].asJSON() isnt @[table.primaryKey].asJSON()
                    return {errors: 1, error: "update cannot change primary key #{table.primaryKey} (got objects #{DemoServer.prototype.convertToJSON(@.asJSON())}, #{DemoServer.prototype.convertToJSON(update_doc.asJSON())})"}
                neu = @.merge mapping @
                table.insert neu, true
                {updated: 1}

            replace: (mapping) ->
                neu = mapping @
                table.insert neu, true
                {modified: 1}

            del: ->
                table.deleteKey @[table.primaryKey].asJSON()
                {deleted: 1}

        proto.__proto__ = obj.__proto__
        obj.__proto__ = proto

        return obj

class RDBPrimitive extends RDBJson
    constructor: (val) -> @data = val
    asJSON: -> @data
    copy: -> new RDBPrimitive @data

    not: -> not @data

    eq: (other) -> @data is other.asJSON()
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
