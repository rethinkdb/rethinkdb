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
        return null

    dropTable: (name) ->
        delete @tables[name]
        return null

    getTable: (name) ->
        @tables[name] || null

    getTableNames: -> (tblN for own tblN of @tables)
