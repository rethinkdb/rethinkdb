goog.provide('rethinkdb.Universe')

class RDBUniverse
	constructor: (keyOrFile) ->
		@dbs = {}

		if keyOrFile?
			@loadFrom keyOrFile
		else
			# Add default database test
			@createDatabase 'test'

	loadFrom: (keyOrFile) ->
		null
		
	saveTo: (keyOrFile) ->
		null

	createDatabase: (dbName) ->
		if @dbs[dbName]? then throw new RuntimeError ""
		else @dbs[dbName] = new RDBDatabase

	dropDatabase: (dbName) ->
        if not @dbs[dbName]? then throw new RuntimeError ""
        else delete @dbs[dbName]

	getDatabase: (dbName) ->
		if @dbs[dbName]? then return @dbs[dbName]
		else return null
	
	listDatabases: -> (dbN for own dbN of @dbs)
