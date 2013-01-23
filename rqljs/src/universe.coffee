goog.provide('rethinkdb.Universe')

class RDBUniverse
    canonicalLocation: "rethinkdb-data"

    constructor: ->
        @dbs = {}

        if not @load()
            # New database, add default database test
            @createDatabase new RDBPrimitive 'test'
            @save()

    load: -> @loadFrom @canonicalLocation

    loadFrom: (keyOrFile) ->
        fs = require('fs')
        try
            strrep = fs.readFileSync(keyOrFile, 'utf8')
        catch err
            return false
        @deserialize strrep
        return true

    save: -> @saveTo @canonicalLocation
        
    saveTo: (keyOrFile) ->
        fs = require('fs')
        strrep = @serialize()
        fs.writeFileSync(keyOrFile, strrep, 'utf8')

    deserialize: (strrep) ->
        objrep = JSON.parse strrep
        for own dbName,db of objrep['dbs']
            @dbs[dbName] = new RDBDatabase db

    serialize: -> "{\"dbs\":{#{("\"#{dbName}\":#{db.serialize()}" for own dbName,db of @dbs)}}}"

    createDatabase: (dbName) ->
        if @dbs[dbName.asJSON()]? then throw new RuntimeError "already exists"
        else @dbs[dbName.asJSON()] = new RDBDatabase
        new RDBObject {'created': 1}

    dropDatabase: (dbName) ->
        if not @dbs[dbName.asJSON()]? then throw new RuntimeError "does not exist"
        else delete @dbs[dbName.asJSON()]
        new RDBObject {'dropped': 1}

    getDatabase: (dbName) ->
        strName = dbName.asJSON()
        unless @dbs[strName]?
            throw new RuntimeError "does not exist"
        else
            return @dbs[strName]
    
    listDatabases: -> new RDBArray ((new RDBPrimitive dbN) for own dbN of @dbs)
