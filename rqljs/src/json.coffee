goog.provide('rethinkdb.RDBType')

class RDBType
    # RQL types are comparable so these numeric mappings are meaningful
    @ARRAY:   0
    @BOOLEAN: 1
    @NULL:    2
    @NUMBER:  3
    @OBJECT:  4
    @STRING:  5

    typeOf: ->
        if goog.isArray(@asJSON())
            RDBType.ARRAY
        else if typeof @asJSON() is 'boolean'
            RDBType.BOOLEAN
        else if @asJSON() is null
            RDBType.NULL
        else if typeof @asJSON() is 'number'
            RDBType.NUMBER
        else if typeof @asJSON() is 'object'
            RDBType.OBJECT
        else if typeof @asJSON() is 'string'
            RDBType.STRING

    asJSON: -> new ServerError "Abstract method"
    copy:   -> new ServerError "Abstract method"

    isNull: -> (@ instanceof RDBPrimitive and @asJSON() is null)

    pick:   (attr) -> throw new RuntimeError "Type Error"
    unpick: (attr) -> throw new RuntimeError "Type Error"
    merge:  (attr) -> throw new RuntimeError "Type Error"
    append: (attr) -> throw new RuntimeError "Type Error"

    union: (other) -> throw new RuntimeError "Type Error"
    count:         -> throw new RuntimeError "Type Error"
    distinct:      -> throw new RuntimeError "Type Error"
    
    not: -> new ServerError "Abstract method"

    # For most types it will work to define only 'eq' and 'lt'
    eq: (other) -> new ServerError "Abstract method"
    lt: (other) -> new ServerError "Abstract method"

    ne: (other) -> new RDBPrimitive (not @eq(other).asJSON())
    ge: (other) -> new RDBPrimitive (not @lt(other).asJSON())
    gt: (other) -> new RDBPrimitive (not @lt(other).asJSON()) and (not @eq(other).asJSON())
    le: (other) -> new RDBPrimitive (not @gt(other).asJSON())

    add: (other) -> new ServerError "Abstract method"
    sub: (other) -> new ServerError "Abstract method"
    mul: (other) -> new ServerError "Abstract method"
    div: (other) -> new ServerError "Abstract method"
    mod: (other) -> new ServerError "Abstract method"

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

class RDBPrimitive extends RDBType
    constructor: (val) -> @data = val
    asJSON: -> @data
    copy: -> new RDBPrimitive @data

    eq: (other) -> new RDBPrimitive (@typeOf() is other.typeOf() and @asJSON() is other.asJSON())
    lt: (other) ->
        if @typeOf() is other.typeOf()
            new RDBPrimitive @data < other.asJSON()
        else
            new RDBPrimitive @typeOf() < other.typeOf()
    
    add: (other) -> new RDBPrimitive @asJSON()+other.asJSON()
    sub: (other) -> new RDBPrimitive @data - other.asJSON()
    mul: (other) -> new RDBPrimitive @data * other.asJSON()
    div: (other) -> new RDBPrimitive @data / other.asJSON()
    mod: (other) -> new RDBPrimitive @data % other.asJSON()

class RDBObject extends RDBType
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
        ks1 = @keys()
        ks2 = other.keys()
        unless ks1.length is ks2.length then return new RDBPrimitive false
        (return new RDBPrimitive false for k1,i in ks1 when k1 isnt ks2[i])
        for k in ks1
            if (@[k].ne other[k]).asJSON() then return false
        return new RDBPrimitive true

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
        for own k,v of other
            self[k] = v
        return self

    pick: (attrs) ->
        self = @copy()
        result = new RDBObject()
        for k,i in attrs
            result[k] = self[k]
        return result

    unpick: (attrs) ->
        self = @copy()
        for k in attrs
            delete self[k]
        return self
