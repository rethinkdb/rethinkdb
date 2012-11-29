goog.provide('rethinkdb.server.RDBDatabase')

goog.require('rethinkdb.server.RDBTable')

# A RQL database
class RDBDatabase
    constructor: ->
        # A database is just a map of table names to tables
        @tables = {}

    createTable: (name, {primaryKey}) ->
        primaryKey ?= 'id'
        @tables[name] = new RDBTable primaryKey
        return []

    dropTable: (name) ->
        delete @tables[name]
        return []

    getTable: (name) ->
        if @tables[name]?
            return @tables[name]
        else
            throw new RuntimeError "Error during operation `EVAL_TABLE #{name}`: No entry with that name."

    getTableNames: -> (tblN for own tblN of @tables)
