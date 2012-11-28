goog.provide('rethinkdb.server.DemoServer')

goog.require('rethinkdb.server.Errors')
goog.require('rethinkdb.server.RDBDatabase')
goog.require('rethinkdb.server.RDBJson')
goog.require('goog.proto2.WireFormatSerializer')
goog.require('Query')

# Local JavaScript server
class DemoServer
    implicitVarId = '__IMPLICIT_VAR__'

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

        # Scope chain for let and lambdas
        @curScope = new RDBLetScope null, {}

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
    log: (msg) -> #console.log msg

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
            unless err instanceof RDBError then throw err
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
        (@evaluateTerm term).asJSON()

    evaluateWriteQuery: (writeQuery) ->
        switch writeQuery.getType()
            when WriteQuery.WriteQueryType.UPDATE
                update = writeQuery.getUpdate()
                mapping = @evaluateMapping update.getMapping()
                view = update.getView()
                (@evaluateTerm view).update(mapping)
            when WriteQuery.WriteQueryType.DELETE
                (@evaluateTerm writeQuery.getDelete().getView()).del()
            when WriteQuery.WriteQueryType.MUTATE
                mutate = writeQuery.getMutate()
                mapping = @evaluateMapping mutate.getMapping()
                view = mutate.getView()
                (@evaluateTerm view).replace(mapping)
            when WriteQuery.WriteQueryType.INSERT
                @evaluateInsert writeQuery.getInsert()
            when WriteQuery.WriteQueryType.FOREACH
                mapping = @evaluateForEach writeQuery.getForEach()
                stream = @evaluateTerm writeQuery.getForEach().getStream()
                stream.forEach mapping
            when WriteQuery.WriteQueryType.POINTUPDATE
                pointUpdate = writeQuery.getPointUpdate()
                table = @getTable pointUpdate.getTableRef()
                record = table.get (@evaluateTerm pointUpdate.getKey()).asJSON()
                mapping = @evaluateMapping pointUpdate.getMapping()
                record.update mapping
                return {updated:1, skipped:0, errors: 0}
            when WriteQuery.WriteQueryType.POINTDELETE
                pointDelete = writeQuery.getPointDelete()
                table = @getTable pointDelete.getTableRef()
                record = table.get (@evaluateTerm pointDelete.getKey()).asJSON()
                record.del()
                return {deleted: 1, skipped:0, errors:0}
            when WriteQuery.WriteQueryType.POINTMUTATE
                pointMutate = writeQuery.getPointMutate()
                table = @getTable pointMutate.getTableRef()
                record = table.get (@evaluateTerm pointMutate.getKey()).asJSON()

                if record.isNull() then throw new RuntimeError "Key not found"

                mapping = @evaluateMapping pointMutate.getMapping()
                record.replace mapping
                return {modified:1, inserted:0, deleted:0, errors: 0}

    evaluateInsert: (insert) ->
        table = @getTable insert.getTableRef()
        upsert = insert.getOverwrite()

        inserted = 0
        errors = 0
        generated_keys = []
        first_error = null

        for term in insert.termsArray()
            {error, generatedKey} = table.insert (@evaluateTerm term), upsert
            if generatedKey? then generated_keys.push generatedKey
            if error?
                errors += 1
                unless first_error then first_error = error
            else
                inserted += 1

        result = {inserted: inserted, errors: errors}
        unless generated_keys.length is 0
            result['generated_keys'] = generated_keys
        if first_error?
            result['first_error'] = first_error

        return result

    evaluateReduction: (reduction) ->
        arg1 = reduction.getVar1()
        arg2 = reduction.getVar2()
        body = reduction.getBody()
        (val1, val2) =>
            binds = {}
            binds[arg1] = val1
            binds[arg2] = val2
            @evaluateWith binds, body

    evaluateMapping: (mapping) ->
        arg = mapping.getArg()
        body = mapping.getBody()
        (val) =>
            binds = {}
            binds[implicitVarId] = arguments[0]
            binds[arg] = val
            @evaluateWith binds, body

    evaluateForEach: (foreach) ->
        arg = foreach.getVar()
        queriesArray = foreach.queriesArray()
        (val) =>
            binds = {}
            binds[implicitVarId] = arguments[0]
            binds[arg] = val
            @curScope = new RDBLetScope @curScope, binds
            results = (@evaluateWriteQuery wq for wq in queriesArray)
            @curScop = @curScope.parent
            results

    evaluateTerm: (term) ->
        switch term.getType()
            when Term.TermType.JSON_NULL
                new RDBPrimitive null
            when Term.TermType.VAR
                @curScope.lookup term.getVar()
            when Term.TermType.LET
                letM = term.getLet()
                binds = {}
                for bind in letM.bindsArray()
                    binds[bind.getVar()] = @evaluateTerm bind.getTerm()
                @evaluateWith binds, letM.getExpr()
            when Term.TermType.CALL
                @evaluateCall term.getCall()
            when Term.TermType.IF
                ifM = term.getIf()
                if (@evaluateTerm ifM.getTest()).asJSON()
                    @evaluateTerm ifM.getTrueBranch()
                else
                    @evaluateTerm ifM.getFalseBranch()
            when Term.TermType.ERROR
                throw new RuntimeError term.getError()

            # Primitive values are really easy since they're all JS types anyway
            when Term.TermType.NUMBER
                new RDBPrimitive term.getNumber()
            when Term.TermType.STRING
                new RDBPrimitive term.getValuestring()
            when Term.TermType.JSON
                JSON.parse term.getJsonstring()
            when Term.TermType.BOOL
                new RDBPrimitive term.getValuebool()
            when Term.TermType.ARRAY
                new RDBArray (@evaluateTerm term for term in term.arrayArray())
            when Term.TermType.OBJECT
                obj = new RDBObject
                for tuple in term.objectArray()
                    obj[tuple.getVar()] = @evaluateTerm tuple.getTerm()
                return obj

            when Term.TermType.GETBYKEY
                gbk = term.getGetByKey()

                table = @getTable gbk.getTableRef()
                unless table then throw new RuntimeError "No such table"

                pkVal = @evaluateTerm gbk.getKey()

                return table.get(pkVal.asJSON())

            when Term.TermType.TABLE
                @getTable(term.getTable().getTableRef())
            when Term.TermType.JAVASCRIPT
                throw new RuntimeError "Not Implemented"
            when Term.TermType.IMPLICIT_VAR
                @curScope.lookup implicitVarId
            else throw new RuntimeError "Unknown term type"

    evaluateWith: (binds, term) ->
        @curScope = new RDBLetScope @curScope, binds
        result = @evaluateTerm term
        @curScop = @curScope.parent
        return result

    getTable: (tableRef) ->
        dbName = tableRef.getDbName()
        tableName = tableRef.getTableName()
        db = @dbs[dbName]
        if db? then db.getTable(tableName)
        else new RDBPrimitive null

    evaluateCall: (call) ->
        builtin = call.getBuiltin()
        args = (@evaluateTerm term for term in call.argsArray())

        switch builtin.getType()
            when Builtin.BuiltinType.NOT
                new RDBPrimitive args[0].not()
            when Builtin.BuiltinType.GETATTR
                args[0][builtin.getAttr()]
            when Builtin.BuiltinType.IMPLICIT_GETATTR
                (@curScope.lookup implicitVarId)[builtin.getAttr()]
            when Builtin.BuiltinType.HASATTR
                new RDBPrimitive args[0][builtin.getAttr()]?
            when Builtin.BuiltinType.IMPLICIT_HASATTR
                (@curScope.lookup implicitVarId)[builtin.getAttr()]?
            when Builtin.BuiltinType.PICKATTRS
                args[0].pick builtin.attrsArray()
            when Builtin.BuiltinType.IMPLICIT_PICKATTRS
                (@curScope.lookup implicitVarId).pick builtin.attrsArray()
            when Builtin.BuiltinType.MAPMERGE
                args[0].merge args[1]
            when Builtin.BuiltinType.ARRAYAPPEND
                args[0].append args[1]
            when Builtin.BuiltinType.SLICE
                args[0].slice(args[1].asJSON(), args[2]?.asJSON() || undefined)
            when Builtin.BuiltinType.ADD
                args.reduce ((a,b)-> new RDBPrimitive a.add b), new RDBPrimitive 0
            when Builtin.BuiltinType.SUBTRACT
                new RDBPrimitive args.reduce ((a,b)->a.sub b)
            when Builtin.BuiltinType.MULTIPLY
                args.reduce ((a,b)-> new RDBPrimitive a.mul b), new RDBPrimitive 1
            when Builtin.BuiltinType.DIVIDE
                new RDBPrimitive args.reduce ((a,b)->a.div b)
            when Builtin.BuiltinType.MODULO
                new RDBPrimitive args.reduce ((a,b)->a.mod b)
            when Builtin.BuiltinType.COMPARE
                @evaluateComarison builtin.getComparison(), args
            when Builtin.BuiltinType.FILTER
                predicate = @evaluateMapping builtin.getFilter().getPredicate()
                args[0].filter predicate
            when Builtin.BuiltinType.MAP
                mapping = @evaluateMapping builtin.getMap().getMapping()
                args[0].map mapping
            when Builtin.BuiltinType.CONCATMAP
                mapping = @evaluateMapping builtin.getConcatMap().getMapping()
                args[0].concatMap mapping
            when Builtin.BuiltinType.ORDERBY
                orderbys = builtin.orderByArray()
                args[0].orderBy orderbys.map (orderby) ->
                    {attr: orderby.getAttr(), asc: orderby.getAscendingOrDefault()}
            when Builtin.BuiltinType.DISTINCT
                args[0].distinct()
            when Builtin.BuiltinType.LENGTH
                args[0].count()
            when Builtin.BuiltinType.UNION
                args[0].union(args[1])
            when Builtin.BuiltinType.NTH
                (args[0].asArray()[args[1].asJSON()])
            when Builtin.BuiltinType.STREAMTOARRAY
                args[0] # This is a meaningless function for us
            when Builtin.BuiltinType.ARRAYTOSTREAM
                args[0] # This is a meaningless function for us
            when Builtin.BuiltinType.REDUCE
                base = @evaluateTerm builtin.getReduce().getBase()
                reduction = @evaluateReduction builtin.getReduce()
                args[0].reduce base, reduction
            when Builtin.BuiltinType.GROUPEDMAPREDUCE
                groupedMapReduce = builtin.getGroupedMapReduce()
                groupMapping = @evaluateMapping   groupedMapReduce.getGroupMapping()
                valueMapping = @evaluateMapping   groupedMapReduce.getValueMapping()
                reduction    = @evaluateReduction groupedMapReduce.getReduction()

                args[0].groupedMapReduce(groupMapping, valueMapping, reduction)
            when Builtin.BuiltinType.ANY
                args.reduce ((a,b) -> new RDBPrimitive (a.asJSON() || b.asJSON())),
                                      new RDBPrimitive false
            when Builtin.BuiltinType.ALL
                args.reduce ((a,b) -> new RDBPrimitive (a.asJSON() && b.asJSON())),
                                      new RDBPrimitive true
            when Builtin.BuiltinType.RANGE
                range = builtin.getRange()
                args[0].between range.getAttrname(),
                                (@evaluateTerm range.getLowerbound()),
                                (@evaluateTerm range.getUpperbound())
            when Builtin.BuiltinType.IMPLICIT_WITHOUT
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.WITHOUT
                args[0].unpick builtin.attrsArray()

    evaluateComarison: (comparison, args) ->
        op = switch comparison
            when Builtin.Comparison.EQ then (a,b) -> a.eq b
            when Builtin.Comparison.NE then (a,b) -> a.ne b
            when Builtin.Comparison.LT then (a,b) -> a.lt b
            when Builtin.Comparison.LE then (a,b) -> a.le b
            when Builtin.Comparison.GT then (a,b) -> a.gt b
            when Builtin.Comparison.GE then (a,b) -> a.ge b
        opReduc = (last, rest) ->
            if !rest.length then return true
            next = rest[0]
            unless op last, next then return false
            opReduc next, rest[1..]
        new RDBPrimitive opReduc args[0], args[1..]

class RDBLetScope
    constructor: (parent, binds) ->
        @parent = parent
        @binds = binds

    lookup: (varName) ->
        if @binds[varName]?
            @binds[varName]
        else if @parent?
            @parent.lookup varName
        else
            null
