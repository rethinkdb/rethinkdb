goog.provide('rethinkdb.server.DemoServer')

goog.require('rethinkdb.server.Errors')
goog.require('rethinkdb.server.RDBDatabase')
goog.require('rethinkdb.server.RDBJson')
goog.require('rethinkdb.server.Utils')
goog.require('goog.proto2.WireFormatSerializer')
goog.require('Query')

# Local JavaScript server
class DemoServer
    implicitVarId = '__IMPLICIT_VAR__'

    constructor: ->
        @descriptor = window.Query.getDescriptor()
        @serializer = new goog.proto2.WireFormatSerializer()

        # Map of databases
        @dbs = {}

        # Scope chain for let and lambdas
        @curScope = new RDBLetScope null, {}

        # Add default database test
        @createDatabase 'test'

        # Used to keep track of the backtrace for the currently executing query
        @bt = []

    # Evaluate op with the given backtrace frame
    frame: (fname, op) ->
        @bt.push fname
        res = op()
        @bt.pop()
        return res

    appendBacktrace: (msg) ->
        msg + "\nBacktrace:\n" + @bt.join('\n') + '\n'

    createDatabase: (name) ->
        Utils.checkName name
        if @dbs[name]?
            throw new RuntimeError "Error during operation `CREATE_DB #{name}`: Entry already exists."
        @dbs[name] = new RDBDatabase
        return []

    dropDatabase: (name) ->
        Utils.checkName name
        if not @dbs[name]?
            throw new RuntimeError "Error during operation `DROP_DB #{name}`: No entry with that name."
        delete @dbs[name]
        return []

    log: (msg) -> #console.log msg

    print_all: ->
        console.log @local_server

    # Validate the length of the received buffer and return the stripped buffer
    validateBuffer: (buffer) ->
        uint8Array = new Uint8Array buffer
        dataView = new DataView buffer
        expectedLength = dataView.getInt32(0, true)
        pb = uint8Array.subarray 4
        if pb.length is expectedLength
            return pb
        else
            throw new RuntimeError "protocol buffer not of the correct length"

    deserializePB: (pbArray) ->
        @serializer.deserialize @descriptor, pbArray

    execute: (data) ->
        query = @deserializePB @validateBuffer data
        @log 'Server: deserialized query'
        @log query

        response = new Response
        response.setToken query.getToken()
        try
            @bt = [] # Make sure this is clear for the new query
            result = JSON.stringify @evaluateQuery query
            if result? then response.addResponse result
            response.setStatusCode Response.StatusCode.SUCCESS_JSON
        catch err
            unless err instanceof RDBError then throw err #debugger
            response.setErrorMessage err.message
            #TODO: other kinds of errors
            if err instanceof RuntimeError
                response.setStatusCode Response.StatusCode.RUNTIME_ERROR
            else if err instanceof BadQuery
                response.setStatusCode Response.StatusCode.BAD_QUERY
            #TODO: backtraces

        @log 'Server: response'
        @log response

        backtrace = new Response.Backtrace
        for frame in @bt
            backtrace.addFrame frame
        response.setBacktrace backtrace

        # Reset bt for the next query
        @bt = []

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
                @frame "table_ref", => @frame "db_name", =>
                    unless db?
                        throw new RuntimeError "Error during operation "+
                                               "`FIND_DATABASE #{tableRef.getDbName()}`: "+
                                               "No entry with that name."
                primaryKey = createTable.getPrimaryKey()
                tableName = tableRef.getTableName()

                db.createTable tableName, {primaryKey: primaryKey}
            when MetaQuery.MetaQueryType.DROP_TABLE
                tableRef = metaQuery.getDropTable()
                db = @dbs[tableRef.getDbName()]
                @frame "db_name", =>
                    unless db?
                        throw new RuntimeError "Error during operation "+
                                               "`FIND_DATABASE #{tableRef.getDbName()}`: "+
                                               "No entry with that name."
                @frame "table_name", => db.dropTable tableRef.getTableName(), tableRef.getDbName()
            when MetaQuery.MetaQueryType.LIST_TABLES
                db = @dbs[metaQuery.getDbName()]
                @frame "db_name", =>
                    unless db?
                        throw new RuntimeError "Error during operation "+
                                               "`FIND_DATABASE #{metaQuery.getDbName()}`: "+
                                               "No entry with that name."
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
                @frame "modify_map", => (@evaluateTerm view).update(mapping, @)
            when WriteQuery.WriteQueryType.DELETE
                (@evaluateTerm writeQuery.getDelete().getView()).del()
            when WriteQuery.WriteQueryType.MUTATE
                mutate = writeQuery.getMutate()
                mapping = @evaluateMapping mutate.getMapping()
                view = mutate.getView()
                @frame "modify_map", => (@evaluateTerm view).replace(mapping, @)
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

                if record.asJSON()?
                    record.update mapping
                    return {updated:1, skipped:0, errors: 0}
                else
                    return {updated: 0, skipped: 1, errors: 0}
            when WriteQuery.WriteQueryType.POINTDELETE
                pointDelete = writeQuery.getPointDelete()
                table = @getTable pointDelete.getTableRef()
                record = table.get (@evaluateTerm pointDelete.getKey()).asJSON()
                if record?.asJSON()?
                    record.del()
                    return {deleted: 1}
                else
                    return {deleted: 0}
            when WriteQuery.WriteQueryType.POINTMUTATE
                result = {modified:0, inserted:0, deleted:0, errors: 0}

                pointMutate = writeQuery.getPointMutate()
                mapping = @evaluateMapping pointMutate.getMapping()
                table = @getTable pointMutate.getTableRef()
                record = table.get (@evaluateTerm pointMutate.getKey()).asJSON()

                res = @frame 'point_map', => record.replace mapping

                switch res
                    when "inserted"
                        result.inserted = 1
                    when "modified"
                        result.modified++
                    when "deleted"
                        result.deleted++
                return result

    evaluateInsert: (insert) ->
        table = @getTable insert.getTableRef()
        upsert = insert.getOverwrite()

        inserted = 0
        errors = 0
        generated_keys = []
        first_error = null

        for term,i in insert.termsArray()
            @frame "term:"+i, =>
                try
                    generatedKey = table.insert (@evaluateTerm term), upsert
                    if generatedKey? then generated_keys.push generatedKey
                    inserted += 1
                catch error
                    errors += 1
                    unless first_error
                        first_error = @appendBacktrace error.message

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
                    binds[bind.getVar()] = @frame "bind:"+bind.getVar(), =>
                        @evaluateTerm bind.getTerm()
                @frame "expr", => @evaluateWith binds, letM.getExpr()
            when Term.TermType.CALL
                @evaluateCall term.getCall()
            when Term.TermType.IF
                ifM = term.getIf()
                testRes = @frame "test", =>
                    res = (@evaluateTerm ifM.getTest())
                    unless res.typeOf() is RDBJson.RDBTypes.BOOLEAN
                        throw new RuntimeError "The IF test must evaluate to a boolean."
                    res
                if testRes.asJSON()
                    @frame "true", => @evaluateTerm ifM.getTrueBranch()
                else
                    @frame "false", => @evaluateTerm ifM.getFalseBranch()
            when Term.TermType.ERROR
                throw new RuntimeError term.getError()

            # Primitive values are really easy since they're all JS types anyway
            when Term.TermType.NUMBER
                if term.getNumber() is Infinity
                    throw new RuntimeError 'Illegal numeric value inf.'
                else if term.getNumber() is -Infinity
                    throw new RuntimeError 'Illegal numeric value -inf.'
                else if term.getNumber() != term.getNumber()
                    throw new RuntimeError 'Illegal numeric value nan.'
                new RDBPrimitive term.getNumber()
            when Term.TermType.STRING
                new RDBPrimitive term.getValuestring()
            when Term.TermType.JSON
                JSON.parse term.getJsonstring()
            when Term.TermType.BOOL
                new RDBPrimitive term.getValuebool()
            when Term.TermType.ARRAY
                new RDBArray (@frame "elem:"+i, (=> @evaluateTerm term) for term,i in term.arrayArray())
            when Term.TermType.OBJECT
                obj = new RDBObject
                for tuple in term.objectArray()
                    obj[tuple.getVar()] = @frame "key:"+tuple.getVar(), => @evaluateTerm tuple.getTerm()
                return obj

            when Term.TermType.GETBYKEY
                gbk = term.getGetByKey()
                attr = gbk.getAttrname()
                table = @getTable gbk.getTableRef()
                
                @frame "attrname", =>
                    if attr isnt table.primaryKey
                        throw new RuntimeError "Attribute: #{attr} is not the primary key "+
                                               "(#{table.primaryKey}) and thus cannot be selected upon."

                pkVal = @frame "key", => @evaluateTerm gbk.getKey()

                return table.get(pkVal.asJSON())

            when Term.TermType.TABLE
                @getTable(term.getTable().getTableRef())
            when Term.TermType.JAVASCRIPT
                # That's not really safe, but if the user want to screw himself, he can.
                result = eval term.getJavascript()
                if result is null
                    new RDBPrimitive result
                else if goog.isArray results
                    new RDBArray result
                else if typeof result is 'object'
                    new RDBOject result
                else
                    new RDBPrimitive result
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
        if db?
            return db.getTable(tableName)
        else
            throw new RuntimeError "Error during operation `EVAL_DB #{dbName}`: No entry with that name."

    evaluateCall: (call) ->
        builtin = call.getBuiltin()
        args = (@frame "arg:"+i, (=> @evaluateTerm term) for term,i in call.argsArray())

        switch builtin.getType()
            when Builtin.BuiltinType.NOT
                if args[0].typeOf() isnt RDBJson.RDBTypes.BOOLEAN
                    @frame "arg:0", =>
                        throw new RuntimeError "Not can only be called on a boolean"
                args[0].not()
            when Builtin.BuiltinType.GETATTR
                @frame "arg:0", =>
                    if args[0].typeOf() isnt RDBJson.RDBTypes.OBJECT
                        throw new RuntimeError "Data: \n#{JSON.stringify(args[0].asJSON())}\nmust be an object"
                if args[0][builtin.getAttr()]?
                    return args[0][builtin.getAttr()]
                else
                    @frame "attr", =>
                        throw new RuntimeError "Object:\n#{Utils.stringify(args[0].asJSON())}\n"+
                                               "is missing attribute #{Utils.stringify(builtin.getAttr())}"
            when Builtin.BuiltinType.IMPLICIT_GETATTR
                (@curScope.lookup implicitVarId)[builtin.getAttr()]
            when Builtin.BuiltinType.HASATTR
                @frame "arg:0", =>
                    if args[0].typeOf() isnt RDBJson.RDBTypes.OBJECT
                        throw new RuntimeError "Data: \n#{JSON.stringify(args[0].asJSON())}\nmust be an object"
                new RDBPrimitive args[0][builtin.getAttr()]?
            when Builtin.BuiltinType.IMPLICIT_HASATTR
                (@curScope.lookup implicitVarId)[builtin.getAttr()]?
            when Builtin.BuiltinType.PICKATTRS
                @frame "arg:0", =>
                    if args[0].typeOf() isnt RDBJson.RDBTypes.OBJECT
                        throw new RuntimeError "Data: \n#{Utils.stringify(args[0].asJSON())}\nmust be an object"
                args[0].pick builtin.attrsArray(), @
            when Builtin.BuiltinType.IMPLICIT_PICKATTRS
                (@curScope.lookup implicitVarId).pick builtin.attrsArray()
            when Builtin.BuiltinType.MAPMERGE
                @frame "arg:0", => if args[0].typeOf() isnt RDBJson.RDBTypes.OBJECT
                    throw new RuntimeError "Data must be an object"
                @frame "arg:1", => if args[1].typeOf() isnt RDBJson.RDBTypes.OBJECT
                    throw new RuntimeError "Data must be an object"
                args[0].merge args[1]
            when Builtin.BuiltinType.ARRAYAPPEND
                @frame "arg:0", => if args[0].typeOf() isnt RDBJson.RDBTypes.ARRAY
                    throw new RuntimeError "The first argument must be an array."
                args[0].append args[1]
            when Builtin.BuiltinType.SLICE
                oneIsNum = args[1].typeOf() is RDBJson.RDBTypes.NUMBER
                oneIsPositive = oneIsNum and args[1].asJSON() >= 0
                oneIsNull = args[1].asJSON() is null
                oneIsNumOrNull = oneIsPositive or oneIsNull

                twoIs = args[2]?
                twoIsNum = twoIs and args[2].typeOf() is RDBJson.RDBTypes.NUMBER
                twoIsPositive = twoIsNum and args[2].asJSON() >= 0
                twoIsNull = twoIs and args[2].asJSON() is null
                twoIsNumOrNull = twoIsPositive or twoIsNull

                @frame "arg:1", =>
                    if not oneIsNumOrNull
                        throw new RuntimeError "Slice start must be null or a nonnegative integer."
                @frame "arg:2", =>
                    if twoIs
                        if not twoIsNumOrNull
                            throw new RuntimeError "Slice stop must be null or a nonnegative integer."
                        if oneIsNum and twoIsNum and (args[2].asJSON() < args[1].asJSON())
                            throw new RuntimeError "Slice stop cannot be before slice start"

                args[0].slice(args[1].asJSON(), args[2]?.asJSON() || undefined)
            when Builtin.BuiltinType.ADD
                @frame "arg:0", =>
                    unless args[0].typeOf() is RDBJson.RDBTypes.NUMBER or
                           args[0].typeOf() is RDBJson.RDBTypes.ARRAY
                        throw new RuntimeError "Can only ADD numbers with numbers and arrays with arrays"
                @frame "arg:1", =>
                    if args[0].typeOf() is   RDBJson.RDBTypes.NUMBER and
                       args[1].typeOf() isnt RDBJson.RDBTypes.NUMBER
                        throw new RuntimeError "Cannot ADD numbers to non-numbers"
                    if args[0].typeOf() is   RDBJson.RDBTypes.ARRAY and
                       args[1].typeOf() isnt RDBJson.RDBTypes.ARRAY
                        throw new RuntimeError "Cannot ADD arrays to non-arrays"
                args[0].add args[1]
            when Builtin.BuiltinType.SUBTRACT
                @frame "arg:0", =>
                    if args[0].typeOf() isnt RDBJson.RDBTypes.NUMBER
                        throw new RuntimeError "All operands of SUBTRACT must be numbers."
                @frame "arg:1", =>
                    if args[1].typeOf() isnt RDBJson.RDBTypes.NUMBER
                        throw new RuntimeError "All operands of SUBTRACT must be numbers."
                args[0].sub args[1]
            when Builtin.BuiltinType.MULTIPLY
                args[0].mul args[1]
            when Builtin.BuiltinType.DIVIDE
                args[0].div args[1]
            when Builtin.BuiltinType.MODULO
                args[0].mod args[1]
            when Builtin.BuiltinType.COMPARE
                @evaluateComarison builtin.getComparison(), args
            when Builtin.BuiltinType.FILTER
                predicate = @evaluateMapping builtin.getFilter().getPredicate()
                @frame "predicate", => args[0].filter predicate
            when Builtin.BuiltinType.MAP
                mapping = @evaluateMapping builtin.getMap().getMapping()
                @frame "mapping", => args[0].map mapping
            when Builtin.BuiltinType.CONCATMAP
                mapping = @evaluateMapping builtin.getConcatMap().getMapping()
                @frame "mapping", => args[0].concatMap mapping
            when Builtin.BuiltinType.ORDERBY
                orderbys = builtin.orderByArray()
                @frame "order_by", => args[0].orderBy orderbys.map (orderby) ->
                    {attr: orderby.getAttr(), asc: orderby.getAscendingOrDefault()}
            when Builtin.BuiltinType.DISTINCT
                @frame "arg:0", => args[0].distinct()
            when Builtin.BuiltinType.LENGTH
                @frame "arg:0", => args[0].count()
            when Builtin.BuiltinType.UNION
                @frame "arg:0", =>
                    if args[0].typeOf() isnt RDBJson.RDBTypes.ARRAY
                        throw new RuntimeError "Required type: array but found #{typeof args[0].asJSON()}."
                @frame "arg:1", =>
                    if args[1].typeOf() isnt RDBJson.RDBTypes.ARRAY
                        throw new RuntimeError "Required type: array but found #{typeof args[1].asJSON()}."
                args[0].union(args[1])
            when Builtin.BuiltinType.NTH
                @frame "arg:1", =>
                    if args[1].typeOf() isnt RDBJson.RDBTypes.NUMBER
                        throw new RuntimeError "The second argument must be an integer."
                    if args[1].asJSON() < 0
                        throw new RuntimeError "The second argument must be a nonnegative integer."
                return args[0].asArray()[args[1].asJSON()]
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

                gm = @evaluateMapping groupedMapReduce.getGroupMapping()
                groupMapping = (v) => @frame "group_mapping", -> gm v
                valueMapping = @evaluateMapping   groupedMapReduce.getValueMapping()
                reduction    = @evaluateReduction groupedMapReduce.getReduction()

                args[0].groupedMapReduce(groupMapping, valueMapping, reduction)
            when Builtin.BuiltinType.ANY
                if not args.every((arg) -> arg.typeOf() is RDBJson.RDBTypes.BOOLEAN)
                    throw new RuntimeError "All operands of ANY must be booleans."
                new RDBPrimitive args.some (arg) -> arg.asJSON()
            when Builtin.BuiltinType.ALL
                if not args.every((arg) -> arg.typeOf() is RDBJson.RDBTypes.BOOLEAN)
                    throw new RuntimeError "All operands of ALL must be booleans."
                new RDBPrimitive args.every (arg) -> arg.asJSON()
            when Builtin.BuiltinType.RANGE
                range = builtin.getRange()

                lb = @frame "lowerbound", =>
                    lb = @evaluateTerm range.getLowerbound()
                    unless lb.typeOf() is RDBJson.RDBTypes.STRING or
                           lb.typeOf() is RDBJson.RDBTypes.NUMBER
                        throw new RuntimeError "Lower bound of RANGE must be a string or a number, not "+
                                "#{Utils.stringify(lb.asJSON())}."
                    else
                        lb
                        

                ub = @frame "upperbound", =>
                    ub = @evaluateTerm range.getUpperbound()
                    unless ub.typeOf() is RDBJson.RDBTypes.STRING or
                           ub.typeOf() is RDBJson.RDBTypes.NUMBER
                        throw new RuntimeError "Upper bound of RANGE must be a string or a number, not "+
                                "#{Utils.stringify(ub.asJSON())}."
                    else
                        ub

                args[0].between range.getAttrname(), lb, ub
            when Builtin.BuiltinType.IMPLICIT_WITHOUT
                throw new RuntimeError "Not implemented"
            when Builtin.BuiltinType.WITHOUT
                @frame "arg:0", =>
                    if args[0].typeOf() isnt RDBJson.RDBTypes.OBJECT
                        throw new RuntimeError "Data: \n#{Utils.stringify(args[0].asJSON())}\nmust be an object"
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
            unless (op last, next).asJSON() then return false
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
            throw new BadQuery "symbol '#{varName}' is not in scope"
