goog.provide('rethinkdb.RDBDatabase')

goog.require('rethinkdb.RDBTable')

# A RQL database
class RDBDatabase
    constructor: ->
        # A database is just a map of table names to tables
        @tables = {}

    createTable: (name, {primaryKey}) ->
        Utils.checkName name
        if @tables[name]?
            throw new RuntimeError "Error during operation `CREATE_TABLE #{name}`: Entry already exists."

        primaryKey ?= 'id'
        @tables[name] = new RDBTable primaryKey
        return []

    dropTable: (name, db_name) ->
        Utils.checkName name
        if not @tables[name]?
            throw new RuntimeError "Error during operation `FIND_TABLE #{db_name}.#{name}`: "+
                                   "No entry with that name."
        delete @tables[name]
        return []

    getTable: (name) ->
        Utils.checkName name
        if @tables[name]?
            return @tables[name]
        else
            throw new RuntimeError "Error during operation `EVAL_TABLE #{name}`: No entry with that name."

    listTables: -> (tblN for own tblN of @tables)
