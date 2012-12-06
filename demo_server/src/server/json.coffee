goog.provide('rethinkdb.server.RDBJson')

class RDBJson
    # RQL types are comparable so these numeric mappings are meaningful
    @RDBTypes =
        ARRAY:   0
        BOOLEAN: 1
        NULL:    2
        NUMBER:  3
        OBJECT:  4
        STRING:  5

    typeOf: ->
        if goog.isArray(@asJSON())
            RDBJson.RDBTypes.ARRAY
        else if typeof @asJSON() is 'boolean'
            RDBJson.RDBTypes.BOOLEAN
        else if @asJSON() is null
            RDBJson.RDBTypes.NULL
        else if typeof @asJSON() is 'number'
            RDBJson.RDBTypes.NUMBER
        else if typeof @asJSON() is 'object'
            RDBJson.RDBTypes.OBJECT
        else if typeof @asJSON() is 'string'
            RDBJson.RDBTypes.STRING

    abstract = new Error "Abstract method"

    isNull: -> (@ instanceof RDBPrimitive and @asJSON() is null)

    pick: (attr) -> throw new RuntimeError "Data: \n#{Utils.stringify(@asJSON())}\nmust be an object"
    unpick: (attr) -> throw new RuntimeError "Data: \n#{Utils.stringify(@asJSON())}\nmust be an object"
    merge: (attr) -> throw new RuntimeError "Data must be an object"
    append: (attr) -> throw new RuntimeError "The first argument must be an array."

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
                    return {errors: 1, error: err.message}
                if update_doc[table.primaryKey]? and
                        (update_doc[table.primaryKey].asJSON() isnt @[table.primaryKey].asJSON())
                    return { errors: 1, error: "update cannot change primary key #{table.primaryKey} "+
                                               "(got objects #{Utils.stringify(@.asJSON())}, "+
                                               "#{Utils.stringify(update_doc.asJSON())})" }
                neu = @.merge mapping @
                table.insert neu, true
                {updated: 1}

            replace: (mapping) ->
                update_doc = mapping @
                if update_doc.asJSON()[table.primaryKey] isnt @asJSON()[table.primaryKey]
                    return { errors: 1, error: "mutate cannot change primary key #{table.primaryKey} "+
                                               "(got objects #{Utils.stringify(@.asJSON())}, "+
                                               "#{Utils.stringify(update_doc.asJSON())})" }
                neu = update_doc
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

    union: -> throw new RuntimeError "Required type: array but found #{typeof @asJSON()}."
    count: -> throw new RuntimeError "LENGTH argument must be an array."
    distinct: -> throw new RuntimeError "Required type: array but found #{typeof @asJSON()}."

    not: -> not @data

    eq: (other) -> @typeOf() is other.typeOf() and @asJSON() is other.asJSON()

    ge: (other) -> not @lt(other)
    le: (other) -> not @gt(other)

    gt: (other) ->
        if @typeOf() is other.typeOf()
            @data > other.asJSON()
        else
            @typeOf() > other.typeOf()

    lt: (other) ->
        if @typeOf() is other.typeOf()
            @data < other.asJSON()
        else
            @typeOf() < other.typeOf()

    add: (other) ->
        if @typeOf() isnt RDBJson.RDBTypes.NUMBER
            throw new RuntimeError "Can only ADD numbers with numbers and arrays with arrays"
        if other.typeOf() isnt RDBJson.RDBTypes.NUMBER
            throw new RuntimeError "Cannot ADD numbers to non-numbers"
        new RDBPrimitive @asJSON()+other.asJSON()

    sub: (other) ->
        if @typeOf() isnt RDBJson.RDBTypes.NUMBER or other.typeOf() isnt RDBJson.RDBTypes.NUMBER
            throw new RuntimeError "All operands of SUBTRACT must be numbers."
        new RDBPrimitive @data - other.asJSON()

    mul: (other) ->
        if @typeOf() isnt RDBJson.RDBTypes.NUMBER or other.typeOf() isnt RDBJson.RDBTypes.NUMBER
            throw new RuntimeError "All operands of MULTIPLY must be numbers."
        new RDBPrimitive @data * other.asJSON()

    div: (other) ->
        if @typeOf() isnt RDBJson.RDBTypes.NUMBER or other.typeOf() isnt RDBJson.RDBTypes.NUMBER
            throw new RuntimeError "All operands of DIVIDE must be numbers."
        new RDBPrimitive @data / other.asJSON()

    mod: (other) ->
        if @typeOf() isnt RDBJson.RDBTypes.NUMBER
            throw new RuntimeError "First operand of MOD must be a number."
        if other.typeOf() isnt RDBJson.RDBTypes.NUMBER
            throw new RuntimeError "Second operand of MOD must be a number."
        new RDBPrimitive @data % other.asJSON()


class RDBObject extends RDBJson
    asJSON: ->
        obj = {}
        for own k,v of @
            obj[k] = v.asJSON()
        return obj

    keys: -> (key for own key of @).sort()

    # Needed to prevent modification of table rows
    copy: ->
        result = new RDBObject
        for own k,v of @
            result[k] = v.copy()
        return result


    eq: (other) ->
        # We should check for attributes of other and make sure it's in this too.
        ((other[k]? && v.eq other[k]) for own k,v of @).reduce (a,b) -> a&&b

    le: (other) -> not @gt(other)
    gt: (other) ->
        if @typeOf() is other.typeOf()
            otherKeys = other.keys()
            for k,i in @keys()
                if k is otherKeys[i]
                    if not @[k].eq(other[k])
                        return @[k].gt(other[k])
                else
                    return k > otherKeys[i]
            return false
        else
            return @typeOf() > other.typeOf()

    ge: (other) -> not @lt(other)
    lt: (other) ->
        if @typeOf() is other.typeOf()
            otherKeys = other.keys()
            for k,i in @keys()
                if k is otherKeys[i]
                    if not @[k].eq(other[k])
                        return @[k].lt(other[k])
                else
                    return k < otherKeys[i]
            return false
        else
            return @typeOf() < other.typeOf()

    merge: (other) ->
        self = @copy()
        if other.typeOf() isnt RDBJson.RDBTypes.OBJECT
            throw new RuntimeError "Data must be an object"
        for own k,v of other
            self[k] = v
        return self

    pick: (attrs) ->
        self = @copy()
        result = new RDBObject
        for k in attrs
            if self[k] is undefined
                throw new RuntimeError "Attempting to pick missing attribute #{k} from data:\n"+
                                       "#{Utils.stringify(self)}"
            result[k] = self[k]
        return result

    unpick: (attrs) ->
        self = @copy()
        for k in attrs
            delete self[k]
        return self
