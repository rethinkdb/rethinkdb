goog.provide('rethinkdb.server.RDBTable')

goog.require('rethinkdb.server.RDBSequence')

# A RQL table
class RDBTable extends RDBSequence
    constructor: (primaryKey) ->
        @primaryKey = primaryKey

        # A table is really just a set of JSON documents.
        # A real rdb table is stored in a b-tree indexed by the
        # primary key. A javascript map (object) is a good enough
        # first approximation.
        @records = {}

    generateUUID = ->
        result = (Math.random()+'').slice(2)+(Math.random()+'').slice(2)
        result = CryptoJS.SHA1(result)+''
        result = result.slice(0, 8)+'-'+
                 result.slice(8,12)+'-'+
                 result.slice(12, 16)+'-'+
                 result.slice(16, 20)+'-'+
                 result.slice(20, 32)
        return result

    insert: (record, upsert=false) ->
        result = null

        pkVal = record[@primaryKey]

        unless pkVal?
            # Generate a key
            id_was_generated = true
            pkVal = generateUUID()
            record[@primaryKey] = new RDBPrimitive pkVal
            result = pkVal
        else
            pkVal = pkVal.asJSON()
            id_was_generated = false

        if typeof pkVal isnt 'string' and typeof pkVal isnt 'number'
            throw new RuntimeError "Cannot insert row #{Utils.stringify(record.asJSON())} "+
                                   "with primary key #{JSON.stringify(pkVal)} of non-string, non-number type."
        else if not upsert and @records[pkVal]? and id_was_generated is true
            throw new RuntimeError "Generated key was a duplicate either you've won "+
                                   "the uuid lottery or you've intentionnaly tried "+
                                   "to predict the keys rdb would generate... in "+
                                   "which case well done."

        else if not upsert and @records[pkVal]? and not id_was_generated
            # That's a cheap hack to match the json of the server.
            # Let's not use colon in our value for now...
            throw new RuntimeError "Duplicate primary key id in #{Utils.stringify(record.asJSON())}"
        else
            @records[pkVal] = record

        # Ensure that this new record is a selection of this table
        RDBSelection::makeSelection record, @

        return result

    get: (pkVal) ->
        if typeof pkVal isnt 'string' and typeof pkVal isnt 'number'
            throw new RuntimeError "Primary key must be a number or a string, not #{Utils.stringify(pkVal)}"
        @records[pkVal] || RDBSelection::makeSelection(new RDBPrimitive(null), @)

    deleteKey: (pkVal) -> delete @records[pkVal]

    asArray: -> (v for k,v of @records)
