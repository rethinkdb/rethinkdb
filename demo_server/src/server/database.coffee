goog.provide('rethinkdb.server.RDBDatabase')

goog.require('rethinkdb.server.RDBTable')
goog.require('rethinkdb.server.Utils')

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

    getTableNames: -> (tblN for own tblN of @tables)
