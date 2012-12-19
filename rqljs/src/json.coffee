goog.provide('rethinkdb.RDBJson')

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
    ne: (other) -> new RDBPrimitive (not (@eq other).asJSON())
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
                updated = mapping @
                neu = @.merge updated
                unless neu[table.primaryKey].eq(@[table.primaryKey]).asJSON()
                    throw new RuntimeError "update cannot change primary key #{table.primaryKey} "+
                                           "(got objects #{Utils.stringify(@.asJSON())}, "+
                                           "#{Utils.stringify(updated.asJSON())})"
                table.insert neu, true
                return true

            replace: (mapping) ->
                replacement = mapping @
                if replacement.isNull()
                    @del()
                    return "deleted"

                if @isNull()
                    table.insert replacement
                    return "inserted"

                if replacement[table.primaryKey].eq(@[table.primaryKey]).asJSON()
                    table.insert replacement, true
                    return "modified"
                    
                throw new RuntimeError "mutate cannot change primary key #{table.primaryKey} "+
                                       "(got objects #{Utils.stringify(@.asJSON())}, "+
                                       "#{Utils.stringify(replacement.asJSON())})"

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

    union: (other) -> throw new RuntimeError "Required type: array but found #{typeof @asJSON()}."
    count: -> throw new RuntimeError "LENGTH argument must be an array."
    distinct: -> throw new RuntimeError "Required type: array but found #{typeof @asJSON()}."

    not: -> new RDBPrimitive not @asJSON()

    eq: (other) -> new RDBPrimitive (@typeOf() is other.typeOf() and @asJSON() is other.asJSON())

    ge: (other) -> new RDBPrimitive (not @lt(other).asJSON())
    le: (other) -> new RDBPrimitive (not @gt(other).asJSON())

    gt: (other) ->
        if @typeOf() is other.typeOf()
            new RDBPrimitive @data > other.asJSON()
        else
            new RDBPrimitive @typeOf() > other.typeOf()

    lt: (other) ->
        if @typeOf() is other.typeOf()
            new RDBPrimitive @data < other.asJSON()
        else
            new RDBPrimitive @typeOf() < other.typeOf()

    add: (other) -> new RDBPrimitive @asJSON()+other.asJSON()

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
    buildFromJS: (jsObject) ->
        for key, value of jsObject
            if value is null
                @[key] = new RDBPrimitive null
            else if typeof value is 'string' or typeof value is 'number' or typeof value is 'boolean'
                @[key] = new RDBPrimitive value
            else if goog.isArray(value)
                arrayValue = new RDBArray
                arrayValue.buildFromJS value
                @[key] = arrayValue
            else if typeof value is 'object'
                objectValue = new RDBObject
                objectValue.buildFromJS value
                @[key] = objectValue
                
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
        new RDBPrimitive (((other[k]? && (v.eq other[k]).asJSON()) for own k,v of @).reduce (a,b) -> a&&b)

    le: (other) -> new RDBPrimitive (not @gt(other).asJSON())
    gt: (other) ->
        if @typeOf() is other.typeOf()
            otherKeys = other.keys()
            for k,i in @keys()
                if k is otherKeys[i]
                    if not @[k].eq(other[k]).asJSON()
                        return @[k].gt(other[k])
                else
                    return new RDBPrimitive k > otherKeys[i]
            return new RDBPrimitive false
        else
            return new RDBPrimitive @typeOf() > other.typeOf()

    ge: (other) -> new RDBPrimitive (not @lt(other).asJSON())
    lt: (other) ->
        if @typeOf() is other.typeOf()
            otherKeys = other.keys()
            for k,i in @keys()
                if k is otherKeys[i]
                    if not @[k].eq(other[k]).asJSON()
                        return @[k].lt(other[k])
                else
                    return new RDBPrimitive k < otherKeys[i]
            return new RDBPrimitive false
        else
            return new RDBPrimitive @typeOf() < other.typeOf()

    merge: (other) ->
        self = @copy()
        if other.typeOf() isnt RDBJson.RDBTypes.OBJECT
            throw new RuntimeError "Data must be an object"
        for own k,v of other
            self[k] = v
        return self

    pick: (attrs, demoServer) ->
        self = @copy()
        result = new RDBObject
        for k,i in attrs
            demoServer.frame "attrs:#{i}", =>
                if self[k] is undefined
                    throw new RuntimeError "Attempting to pick missing attribute #{k} from data:\n"+
                                           "#{Utils.stringify(@asJSON())}"
            result[k] = self[k]
        return result

    unpick: (attrs) ->
        self = @copy()
        for k in attrs
            delete self[k]
        return self
