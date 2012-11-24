goog.provide('rethinkdb.server.RDBTable')

goog.require('rethinkdb.server.RDBSelection')

# A RQL table
class RDBTable extends RDBSelection
    constructor: (primaryKey) ->
        @primaryKey = primaryKey

        # A table is really just a set of JSON documents.
        # A real rdb table is stored in a b-tree indexed by the
        # primary key. A javascript map (object) is a good enough
        # first approximation.
        @records = {}

    insert: (record, upsert=false) ->
        pkVal = record[@primaryKey]?.asJSON()
        if not pkVal?
            throw new RuntimeError "Record does not contain primary key"
        
        if not upsert and @records[pkVal]?
            return 0

        @records[pkVal] = record
        return 1

    get: (pkVal) -> @records[pkVal] || new RDBPrimitive null

    asArray: -> (v for k,v of @records)
