goog.provide('rethinkdb.RDBDatabase')

goog.require('rethinkdb.RDBTable')

# A RQL database
class RDBDatabase
    constructor: (objrep) ->
        # A database is just a map of table names to tables
        @tables = {}

        if objrep?
            for own tableName,table of objrep['tables']
                @tables[tableName] = new RDBTable table['primaryKey'], table['records']

    serialize: -> "{\"tables\":{#{("\"#{tableName}\":#{table.serialize()}" for own tableName,table of @tables)}}}"

    createTable: (name, options) ->
        primary_key = (options['primary_key'] or new RDBPrimitive 'id').asJSON()
        datacenter = (options['datacenter'] or new RDBPrimitive null).asJSON() # Ignored
        cache_size = (options['cache_size'] or new RDBPrimitive null).asJSON() # Ignored

        name = name.asJSON()
        if @tables[name]? then throw new RqlRuntimeError "Table \"#{name}\" already exists."
        @tables[name] = new RDBTable primary_key
        new RDBObject {'created': 1}

    dropTable: (name) ->
        name = name.asJSON()
        unless @tables[name]? then throw new RqlRuntimeError "Table \"#{name}\" does not exist."
        delete @tables[name]
        new RDBObject {'dropped': 1}

    getTable: (name) ->
        name = name.asJSON()

        validateName("Table", name)

        if @tables[name]?
            return @tables[name]
        else
            throw new RqlRuntimeError "Table \"#{name}\" does not exist."

    listTables: -> new RDBArray ((new RDBPrimitive tblN) for own tblN of @tables)
