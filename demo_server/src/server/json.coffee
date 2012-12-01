goog.provide('rethinkdb.server.RDBJson')

class RDBJson
    abstract = new Error "Abstract method"

    isNull: -> (@ instanceof RDBPrimitive and @asJSON() is null)

    pick: (attr) -> throw new RuntimeError "Data: \n#{DemoServer.prototype.convertToJSON(@asJSON())}\nmust be an object"
    unpick: (attr) -> throw new RuntimeError "Data: \n#{DemoServer.prototype.convertToJSON(@asJSON())}\nmust be an object"
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
                if update_doc[table.primaryKey]? and update_doc[table.primaryKey].asJSON() isnt @[table.primaryKey].asJSON()
                    return {errors: 1, error: "update cannot change primary key #{table.primaryKey} (got objects #{DemoServer.prototype.convertToJSON(@.asJSON())}, #{DemoServer.prototype.convertToJSON(update_doc.asJSON())})"}
                neu = @.merge mapping @
                table.insert neu, true
                {updated: 1}

            replace: (mapping) ->
                update_doc = mapping @
                if update_doc.asJSON()[table.primaryKey] isnt @asJSON()[table.primaryKey]
                    return {errors: 1, error: "mutate cannot change primary key #{table.primaryKey} (got objects #{DemoServer.prototype.convertToJSON(@.asJSON())}, #{DemoServer.prototype.convertToJSON(update_doc.asJSON())})"}

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

    eq: (other) ->
        # We are not checking for prototype
        other_value = other.asJSON()
        if typeof @data isnt typeof other_value
            return false
        else if Object.prototype.toString.call(@data) is '[object Array]' and Object.prototype.toString.call(other_value) isnt '[object Array]'
            return false
        else if Object.prototype.toString.call(other_value) is '[object Array]' and Object.prototype.toString.call(@data) isnt '[object Array]'
            return false

        # The two variables have the same type
        if typeof @data is 'number' or typeof @data is 'string' or typeof @data is 'boolean' or typeof @data is 'undefined'
            return @data is other_value
        # Cannot be an array
        else if @data is null
            return @data is other.asJSON()
        else if typeof @data is 'object'
            for own key, valie of @data
                if other_value[key] is undefined
                    return false
            for own key, valie of other_value
                if @data[key].eq(other_value[key]) is false
                    return false
            return true

        @data is other.asJSON()
    ge: (other) -> not @lt(other)
    le: (other) -> not @gt(other)
    gt: (other) ->
        if DemoServer.prototype.convertTypeToNumber(@data) is DemoServer.prototype.convertTypeToNumber(other.asJSON())
            # We cannot have an array here (since it would be in RDBArray)
            return @data > other.asJSON()
        else if DemoServer.prototype.convertTypeToNumber(@data) > DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return true
        else if DemoServer.prototype.convertTypeToNumber(@data) < DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return false
    lt: (other) ->
        if DemoServer.prototype.convertTypeToNumber(@data) is DemoServer.prototype.convertTypeToNumber(other.asJSON())
            # We cannot have an array here (since it would be in RDBArray)
            return @data < other.asJSON()
        else if DemoServer.prototype.convertTypeToNumber(@data) > DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return false
        else if DemoServer.prototype.convertTypeToNumber(@data) < DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return true

    add: (other) ->
        if typeof @asJSON() isnt 'number'
            throw new RuntimeError "Can only ADD numbers with numbers and arrays with arrays"

        if typeof @asJSON() is 'number' and typeof other.asJSON() isnt 'number'
            throw new RuntimeError "Cannot ADD numbers to non-numbers"

        new RDBPrimitive @asJSON()+other.asJSON()
    sub: (other) ->
        if typeof @data isnt 'number' or typeof other.asJSON() isnt 'number'
            throw new RuntimeError "All operands of SUBTRACT must be numbers."
        new RDBPrimitive @data - other.asJSON()
    mul: (other) ->
        if typeof @data isnt 'number' or typeof other.asJSON() isnt 'number'
            throw new RuntimeError "All operands of MULTIPLY must be numbers."
        new RDBPrimitive @data * other.asJSON()
    div: (other) ->
        if typeof @data isnt 'number' or typeof other.asJSON() isnt 'number'
            throw new RuntimeError "All operands of DIVIDE must be numbers."
        new RDBPrimitive @data / other.asJSON()
    mod: (other) ->
        if typeof @data isnt 'number'
            throw new RuntimeError "First operand of MOD must be a number."
        if typeof other.asJSON() isnt 'number'
            throw new RuntimeError "Second operand of MOD must be a number."
        new RDBPrimitive @data % other.asJSON()


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
        # We should check for attributes of other and make sure it's in this too.
        ((other[k]? && v.eq other[k]) for own k,v of @).reduce (a,b) -> a&&b

    le: (other) -> not @gt(other)
    gt: (other) ->
        if DemoServer.prototype.convertTypeToNumber(@) is DemoServer.prototype.convertTypeToNumber(other.asJSON())
            throw new RuntimeError "You cannot compare two objects except for equality."
        else if DemoServer.prototype.convertTypeToNumber(@) > DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return true
        else if DemoServer.prototype.convertTypeToNumber(@) < DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return false
    ge: (other) -> not @lt(other)
    lt: (other) ->
        if DemoServer.prototype.convertTypeToNumber(@) is DemoServer.prototype.convertTypeToNumber(other.asJSON())
            throw new RuntimeError "You cannot compare two objects except for equality."
        else if DemoServer.prototype.convertTypeToNumber(@) > DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return false
        else if DemoServer.prototype.convertTypeToNumber(@) < DemoServer.prototype.convertTypeToNumber(other.asJSON())
            return true

    merge: (other) ->
        self = @copy()
        if typeof other.asJSON() isnt 'object'
            throw new RuntimeError "Data must be an object"
        else if other.asJSON() is null or Object.prototype.toString.call(other.asJSON()) is '[object Array]'
            throw new RuntimeError "Data must be an object"

        for own k,v of other
            self[k] = v
        return self

    pick: (attrs) ->
        self = @copy()
        result = new RDBObject
        for k in attrs
            if self[k] is undefined
                throw new RuntimeError "Attempting to pick missing attribute #{k} from data:\n#{DemoServer.prototype.convertToJSON(self)}"
            result[k] = self[k]
        return result

    unpick: (attrs) ->
        self = @copy()
        for k in attrs
            delete self[k]
        return self
