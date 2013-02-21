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
        if haveFS()
            fs = require('fs')
            try
                strrep = fs.readFileSync(keyOrFile, 'utf8')
            catch err
                return false
        else if haveLocalStorage()
            strrep = localStorage.getItem(keyOrFile)
            unless strrep?
                return false
        else
            throw new ServerError "Cannot load database, no file system or local storage"

        @deserialize strrep
        return true

    save: -> @saveTo @canonicalLocation
        
    saveTo: (keyOrFile) ->
        strrep = @serialize()
        if haveFS()
            fs = require('fs')
            fs.writeFileSync(keyOrFile, strrep, 'utf8')
        else if haveLocalStorage()
            localStorage.setItem(keyOrFile, strrep)
        else
            throw new ServerError "Cannot load database, no file system or local storage"

    deserialize: (strrep) ->
        objrep = JSON.parse strrep
        for own dbName,db of objrep['dbs']
            @dbs[dbName] = new RDBDatabase db

    serialize: -> "{\"dbs\":{#{("\"#{dbName}\":#{db.serialize()}" for own dbName,db of @dbs)}}}"

    createDatabase: (dbName) ->
        if @dbs[dbName.asJSON()]? then throw new RqlRuntimeError "already exists"
        else @dbs[dbName.asJSON()] = new RDBDatabase
        new RDBObject {'created': 1}

    dropDatabase: (dbName) ->
        if not @dbs[dbName.asJSON()]? then throw new RqlRuntimeError "does not exist"
        else delete @dbs[dbName.asJSON()]
        new RDBObject {'dropped': 1}

    getDatabase: (dbName) ->
        strName = dbName.asJSON()
        unless @dbs[strName]?
            throw new RqlRuntimeError "does not exist"
        else
            return @dbs[strName]
    
    listDatabases: -> new RDBArray ((new RDBPrimitive dbN) for own dbN of @dbs)

    haveFS = -> typeof require isnt 'undefined' and require('fs')?
    haveLocalStorage = -> typeof localStorage isnt 'undefined'
