goog.provide('rethinkdb.RDBTable')

goog.require('rethinkdb.RDBSequence')

# A RQL table
class RDBTable extends RDBSequence
    constructor: (primaryKey, records) ->
        @primaryKey = primaryKey

        # A table is really just a set of JSON documents.
        # A real rdb table is stored in a b-tree indexed by the
        # primary key. A javascript map (object) is a good enough
        # first approximation.
        @records = {}

        if records?
            @insert(new RDBArray (new RDBObject row for own k,row of records))

    serialize: -> "{\"primaryKey\":\"#{@primaryKey}\",
\"records\":{#{("\"#{id}\":#{record.serialize()}" for own id,record of @records)}}}"

    generateUUID = ->
        result = (Math.random()+'').slice(2)+(Math.random()+'').slice(2)
        result = CryptoJS.SHA1(result)+''
        result = result.slice(0, 8)+'-'+
                 result.slice(8,12)+'-'+
                 result.slice(12, 16)+'-'+
                 result.slice(16, 20)+'-'+
                 result.slice(20, 32)
        return result

    insert: (records, upsert=false) ->
        result = new RDBObject

        if records.typeOf() == RDBType.OBJECT
            records = new RDBArray [records]

        inserted = 0
        for record in records.asArray()
            pkVal = record[@primaryKey]

            unless pkVal?
                # Generate a key
                id_was_generated = true
                pkVal = generateUUID()
                record[@primaryKey] = new RDBPrimitive pkVal
                #result = pkVal
            else
                pkVal = pkVal.asJSON()
                id_was_generated = false

            @records[pkVal] = record

            # Ensure that this new record is a selection of this table
            RDBSelection::makeSelection record, @
            inserted++

        return new RDBObject {'inserted':inserted}

    get: (pkVal) -> @records[pkVal.asJSON()] || RDBSelection::makeSelection(new RDBPrimitive(null), @)

    deleteKey: (pkVal) -> delete @records[pkVal]

    asArray: -> (v for k,v of @records)
