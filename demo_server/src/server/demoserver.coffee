goog.provide('rethinkdb.server.DemoServer')

goog.require('rethinkdb.server.Errors')
goog.require('rethinkdb.server.RDBDatabase')
goog.require('goog.proto2.WireFormatSerializer')
goog.require('Query')

# Local JavaScript server
class DemoServer
    constructor: ->
        #BC: The way you construct an empty object and then pass it into
        #    wrapper classes for each protobuf class is very weird.
        #    I'm going to invert this inverted structure so that the server
        #    executes each protobuf element and modifies its own state as it goes
        #@local_server = {}

        #BC: There is no reason to separate these into separate methods
        @descriptor = window.Query.getDescriptor()
        @serializer = new goog.proto2.WireFormatSerializer()

        #BC: Here we set up the internal state of the server in a way that allows
        #    people reading this constructor to understand how data in the server
        #    is represented.

        # Map of databases
        @dbs = {}

        # Add default database test
        @createDatabase 'test'

    createDatabase: (name) ->
        @dbs[name] = new RDBDatabase
        return null

    dropDatabase: (name) ->
        delete @dbs[name]
        return null

    #BC: The server should log, yes, but abstracting log allows us to
    #    turn it off, redirect it from the console, add timestamps, etc.
    log: (msg) -> console.log msg

    print_all: ->
        console.log @local_server

    # Validate the length of the received buffer and return the stripped buffer
    validateBuffer: (buffer) ->
        #BC: js/cs uses camelCase by convention
        uint8Array = new Uint8Array buffer
        dataView = new DataView buffer
        expectedLength = dataView.getInt32(0, true)
        pb = uint8Array.subarray 4
        if pb.length is expectedLength
            return pb
        else
            #BC: We need to agree on a way to do errors
            throw new RuntimeError "protocol buffer not of the correct length"

    deserializePB: (pbArray) ->
        @serializer.deserialize @descriptor, pbArray

    #BC: class methods need not be bound (fat arrow)
    execute: (data) ->
        query = @deserializePB @validateBuffer data
        @log 'Server: deserialized query'
        @log query

        #BC: Your Query class conflicts with the Query class in the protobuf 
        #    Be careful not to shadow names like this
        #    In any case, because of the restructure, this is going away
        #query = new Query data_query

        response = new Response
        response.setToken query.getToken()
        try
            result = JSON.stringify @evaluateQuery query
            if result? then response.addResponse result
            response.setStatusCode Response.StatusCode.SUCCESS_JSON
        catch err
            unless err instanceof RuntimeError then throw err
            response.setErrorMessage err.message
            #TODO: other kinds of errors
            response.setStatusCode Response.StatusCode.RUNTIME_ERROR
            #TODO: backtraces

        @log 'Server: response'
        @log response

        # Serialize to protobuf format
        pbResponse = @serializer.serialize response
        length = pbResponse.byteLength

        # Add the length
        #BC: Use a dataview object to write binary representation of numbers
        #    into an array buffer
        finalPb = new Uint8Array(length + 4)
        dataView = new DataView finalPb.buffer
        dataView.setInt32 0, length, true

        finalPb.set pbResponse, 4

        @log 'Server: serialized response'
        @log finalPb
        return finalPb

    evaluateQuery: (query) ->
        #BC: switch statements usually have the 'case' line up with the switch.
        #BC: it is much better to pull the cases from the enum than to evaluate
        #    the enum values your self. This mapping is fragile and can break
        #    if the arbitray enum values change.
        switch query.getType()
            when Query.QueryType.READ
                @evaluateReadQuery query.getReadQuery()
            when Query.QueryType.WRITE
                @evaluateWriteQuery query.getWriteQuery()
            when Query.QueryType.CONTINUE
                throw new RuntimeError "Not Impelemented"
            when Query.QueryType.STOP
                throw new RuntimeError "Not Impelemented"
            when Query.QueryType.META
                @evaluateMetaQuery query.getMetaQuery()
            #BC: switch statements should always contain a default
            else throw new RuntimeError "Unknown Query type"

    evaluateMetaQuery: (metaQuery) ->
        switch metaQuery.getType()
            when MetaQuery.MetaQueryType.CREATE_DB
                @createDatabase metaQuery.getDbName()
            when MetaQuery.MetaQueryType.DROP_DB
                @dropDatabase metaQuery.getDbName()
            when MetaQuery.MetaQueryType.LIST_DBS
                (dbName for own dbName of @dbs)
            when MetaQuery.MetaQueryType.CREATE_TABLE
                createTable = metaQuery.getCreateTable()
                tableRef = createTable.getTableRef()

                db = @dbs[tableRef.getDbName()]
                unless db? then throw new RuntimeError "No such database"
                primaryKey = createTable.getPrimaryKey()
                tableName = tableRef.getTableName()

                db.createTable tableName, {primaryKey: primaryKey}
            when MetaQuery.MetaQueryType.DROP_TABLE
                tableRef = metaQuery.getDropTable()
                db = @dbs[tableRef.getDbName()]
                db.dropTable tableRef.getTableName()
            when MetaQuery.MetaQueryType.LIST_TABLES
                db = @dbs[metaQuery.getDbName()]
                db.getTableNames()

    evaluateReadQuery: (readQuery) ->
        # The only part of a read query that concerns us is the single Term
        term = readQuery.getTerm()
        @evaluateTerm term

    evaluateWriteQuery: (writeQuery) ->
        switch writeQuery.getType()
            when WriteQuery.WriteQueryType.UPDATE
                throw new RuntimeError "Not Implemented"
            when WriteQuery.WriteQueryType.DELETE
                throw new RuntimeError "Not Implemented"
            when WriteQuery.WriteQueryType.MUTATE
                throw new RuntimeError "Not Implemented"
            when WriteQuery.WriteQueryType.INSERT
                @evaluateInsert writeQuery.getInsert()
            when WriteQuery.WriteQueryType.FOREACH
                throw new RuntimeError "Not Implemented"
            when WriteQuery.WriteQueryType.POINTUPDATE
                throw new RuntimeError "Not Implemented"
            when WriteQuery.WriteQueryType.POINTDELETE
                throw new RuntimeError "Not Implemented"
            when WriteQuery.WriteQueryType.POINTMUTATE
                throw new RuntimeError "Not Implemented"

    evaluateInsert: (insert) ->
        table = @getTable insert.getTableRef()
        upsert = insert.getOverwrite()
        inserted = 0
        for term in insert.termsArray()
            inserted += table.insert (@evaluateTerm term), upsert
        return {inserted: inserted}

    evaluateTerm: (term) ->
        switch term.getType()
            when Term.TermType.JSON_NULL
                null
            when Term.TermType.VAR
                null #TODO: ...
            when Term.TermType.LET
                null #TODO: ...
            when Term.TermType.CALL
                @evaluateCall term.getCall()
            when Term.TermType.IF
                null #TODO: ...
            when Term.TermType.ERROR
                throw new RuntimeError term.getError()

            # Primitive values are really easy since they're all JS types anyway
            when Term.TermType.NUMBER
                term.getNumber()
            when Term.TermType.STRING
                term.getValuestring()
            when Term.TermType.JSON
                JSON.parse term.getJsonstring()
            when Term.TermType.BOOL
                term.getValuebool()
            when Term.TermType.ARRAY
                (@evaluateTerm term for term in term.arrayArray())
            when Term.TermType.OBJECT
                obj = {}
                for tuple in term.objectArray()
                    obj[tuple.getVar()] = @evaluateTerm tuple.getTerm()
                return obj

            when Term.TermType.GETBYKEY
                gbk = term.getGetByKey()

                table = @getTable gbk.getTableRef()
                unless table then throw new RuntimeError "No such table"

                pkVal = @evaluateTerm gbk.getKey()

                table.get(pkVal)

            when Term.TermType.TABLE
                @getTable(term.getTable().getTableRef()).asArray()
            when Term.TermType.JAVASCRIPT
                null #TODO: ... eval anyone? or probably function constructor
            when Term.TermType.IMPLICIT_VAR
                null #TODO: ...
            else throw new RuntimeError "Unknown term type"

    getTable: (tableRef) ->
        dbName = tableRef.getDbName()
        tableName = tableRef.getTableName()
        db = @dbs[dbName]
        if db? then db.getTable(tableName)
        else null

    evaluateCall: (call) ->
        builtin = call.getBuiltin()
        args = (@evaluateTerm term for term in call.argsArray())

        switch builtin.getType()
            when Builtin.BuiltinType.NOT
                not args[0]
            when Builtin.BuiltinType.GETATTR
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.IMPLICIT_GETATTR
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.HASATTR
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.IMPLICIT_HASATTR
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.PICKATTRS
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.IMPLICIT_PICKATTRS
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.MAPMERGE
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.ARRAYAPPEND
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.SLICE
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.ADD
                args.reduce (a,b)->a+b
            when Builtin.BuiltinType.SUBTRACT
                args.reduce (a,b)->a-b
            when Builtin.BuiltinType.MULTIPLY
                args.reduce (a,b)->a*b
            when Builtin.BuiltinType.DIVIDE
                args.reduce (a,b)->a/b
            when Builtin.BuiltinType.MODULO
                args.reduce (a,b)->a%b
            when Builtin.BuiltinType.COMPARE
                @evaluateComarison builtin.getComparison(), args
            when Builtin.BuiltinType.FILTER
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.MAP
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.CONCATMAP
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.ORDERBY
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.DISTINCT
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.LENGTH
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.UNION
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.NTH
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.STREAMTOARRAY
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.ARRAYTOSTREAM
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.REDUCE
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.GROUPEDMAPREDUCE
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.ANY
                args.reduce (a,b)->a||b
            when Builtin.BuiltinType.ALL
                args.reduce (a,b)->a&&b
            when Builtin.BuiltinType.RANGE
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.IMPLICIT_WITHOUT
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.WITHOUT
                throw new RuntimeError "Not implemented"

    objEq = (one, two) ->
        unless typeof one is 'object'
            one is two
        else
            (objEq one[k], two[k] for own k of one).reduce (a,b) -> a&&b

    evaluateComarison: (comparison, args) ->
        op = switch comparison
            when Builtin.Comparison.EQ then objEq
            when Builtin.Comparison.NE then (a,b) -> not objEq(a,b)
            when Builtin.Comparison.LT then (a,b) -> a < b
            when Builtin.Comparison.LE then (a,b) -> a <= b
            when Builtin.Comparison.GT then (a,b) -> a > b
            when Builtin.Comparison.GE then (a,b) -> a >= b
        opReduc = (last, rest) ->
            if !rest.length then return true
            next = rest[0]
            unless op last, next then return false
            opReduc next, rest[1..]
        opReduc args[0], args[1..]

    evaluateArithmatic: (op, args) -> args.reduce op
