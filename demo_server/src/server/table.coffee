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
        result = {}

        pkVal = record[@primaryKey]

        unless pkVal?
            # Generate a key
            pkVal = generateUUID()
            record[@primaryKey] = new RDBPrimitive pkVal
            result['generatedKey'] = pkVal
        else
            pkVal = pkVal.asJSON()

        if not upsert and @records[pkVal]?
            result['error'] = "Generated key was a duplicate either you've won "+
                              "the uuid lottery or you've intentionnaly tried "+
                              "to predict the keys rdb would generate... in "+
                              "which case well done."
        else
            @records[pkVal] = record

        # Ensure that this new record is a selection of this table
        RDBSelection::makeSelection record, @

        return result

    get: (pkVal) -> @records[pkVal] || new RDBPrimitive null

    del: (pkVal) -> delete @records[pkVal]

    asArray: -> (v for k,v of @records)
