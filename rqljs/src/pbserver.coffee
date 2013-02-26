goog.provide('rethinkdb.PbServer')

goog.require('rethinkdb.Errors')
goog.require('rethinkdb.RDBDatabase')
goog.require('rethinkdb.Universe')
goog.require('rethinkdb.Context')
goog.require('rethinkdb.RDBType')
goog.require('rethinkdb.AST')
goog.require('goog.proto2.WireFormatSerializer')
goog.require('Query2')

class RDBPbServer
    constructor: ->
        @serializer = new goog.proto2.WireFormatSerializer()

        # Universe of databases we live in
        @universe = new RDBUniverse()
        
    # Validate the length of the received buffer and return the stripped buffer
    validateBuffer: (buffer) ->
        uint8Array = new Uint8Array buffer
        dataView = new DataView buffer
        expectedLength = dataView.getInt32(0, true)
        pb = uint8Array.subarray 4
        if pb.length is expectedLength
            return pb
        else
            throw new RqlRuntimeError "protocol buffer not of the correct length"

    deserializePB: (pbArray) ->
        @serializer.deserialize Query2.getDescriptor(), pbArray

    execute: (data) ->
        query = @deserializePB @validateBuffer data

        response = new Response2
        response.setToken query.getToken()

        ast = @buildQuery query

        # The context in which we will execute this query
        context = new RDBContext @universe

        try
            result = ast.eval(context)

            if result instanceof RDBSequence
                response.setType Response2.ResponseType.SUCCESS_SEQUENCE
                result = result.asArray()
            else
                response.setType Response2.ResponseType.SUCCESS_ATOM
                result = [result]
            for datum in result
                response.addResponse @deconstructDatum datum
        catch err
            unless err instanceof RqlError then throw err

            response.setType (
                if err instanceof RqlRuntimeError
                    Response2.ResponseType.RUNTIME_ERROR
                else if err instanceof RqlCompileError
                    Response2.ResponseType.COMPILE_ERROR
                else if err instanceof RqlClientError
                    Response2.ResponseType.CLIENT_ERROR
                )

            response.addResponse @deconstructDatum new RDBPrimitive err.message
            for f in err.backtrace
                frame = new Response2.Frame

                if typeof f is 'number'
                    frame.setType Response2.Frame.FrameType.POS
                    frame.setPos ''+f
                else if typeof f is 'string'
                    frame.setType Response2.Frame.FrameType.OPT
                    frame.setOpt f
                else throw new ServerError "Unexpected type in backtrace"

                response.addBacktrace frame

        # Serialize to protobuf format
        pbResponse = @serializer.serialize response
        length = pbResponse.byteLength

        # Add the length
        finalPb = new Uint8Array(length + 4)
        dataView = new DataView finalPb.buffer
        dataView.setInt32 0, length, true

        finalPb.set pbResponse, 4
        return finalPb

    buildQuery: (query) ->
        switch query.getType()
            when Query2.QueryType.START
                @buildTerm query.getQuery()
            when Query2.QueryType.CONTINUE
                throw new RqlRuntimeError "Not implemented"
            when Query2.QueryType.STOP
                throw new RqlRuntimeError "Not implemented"

    buildTerm: (term) ->
        if term.getType() is Term2.TermType.DATUM
            return @buildDatum term.getDatum()

        # Else builtin function
        opClass = switch term.getType()
            when Term2.TermType.MAKE_ARRAY   then RDBMakeArray
            when Term2.TermType.MAKE_OBJ     then RDBMakeObj
            when Term2.TermType.VAR          then RDBVar
            when Term2.TermType.JAVASCRIPT   then RDBJavaScript
            when Term2.TermType.ERROR        then RDBUserError
            when Term2.TermType.IMPLICIT_VAR then RDBImplicitVar
            when Term2.TermType.DB           then RDBDBRef
            when Term2.TermType.TABLE        then RDBTableRef
            when Term2.TermType.GET          then RDBGetByKey
            when Term2.TermType.EQ           then RDBEq
            when Term2.TermType.NE           then RDBNe
            when Term2.TermType.LT           then RDBLt
            when Term2.TermType.LE           then RDBLe
            when Term2.TermType.GT           then RDBGt
            when Term2.TermType.GE           then RDBGe
            when Term2.TermType.NOT          then RDBNot
            when Term2.TermType.ADD          then RDBAdd
            when Term2.TermType.SUB          then RDBSub
            when Term2.TermType.MUL          then RDBMul
            when Term2.TermType.DIV          then RDBDiv
            when Term2.TermType.MOD          then RDBMod
            when Term2.TermType.APPEND       then RDBAppend
            when Term2.TermType.SLICE        then RDBSlice
            when Term2.TermType.SKIP         then RDBSkip
            when Term2.TermType.LIMIT        then RDBLimit
            when Term2.TermType.GETATTR      then RDBGetAttr
            when Term2.TermType.CONTAINS     then RDBContains
            when Term2.TermType.PLUCK        then RDBPluck
            when Term2.TermType.WITHOUT      then RDBWithout
            when Term2.TermType.MERGE        then RDBMerge
            when Term2.TermType.BETWEEN      then RDBBetween
            when Term2.TermType.REDUCE       then RDBReduce
            when Term2.TermType.MAP          then RDBMap
            when Term2.TermType.FILTER       then RDBFilter
            when Term2.TermType.CONCATMAP    then RDBConcatMap
            when Term2.TermType.ORDERBY      then RDBOrderBy
            when Term2.TermType.DISTINCT     then RDBDistinct
            when Term2.TermType.COUNT        then RDBCount
            when Term2.TermType.UNION        then RDBUnion
            when Term2.TermType.NTH          then RDBNth
            when Term2.TermType.GROUPED_MAP_REDUCE then RDBGroupedMapReduce
            when Term2.TermType.GROUPBY      then RDBGroupBy
            when Term2.TermType.INNER_JOIN   then RDBInnerJoin
            when Term2.TermType.OUTER_JOIN   then RDBOuterJoin
            when Term2.TermType.EQ_JOIN      then RDBEqJoin
            when Term2.TermType.ZIP          then RDBZip
            when Term2.TermType.COERCE       then RDBCoerce
            when Term2.TermType.TYPEOF       then RDBTypeOf
            when Term2.TermType.UPDATE       then RDBUpdate
            when Term2.TermType.DELETE       then RDBDelete
            when Term2.TermType.REPLACE      then RDBReplace
            when Term2.TermType.INSERT       then RDBInsert
            when Term2.TermType.DB_CREATE    then RDBDbCreate
            when Term2.TermType.DB_DROP      then RDBDbDrop
            when Term2.TermType.DB_LIST      then RDBDbList
            when Term2.TermType.TABLE_CREATE then RDBTableCreate
            when Term2.TermType.TABLE_DROP   then RDBTableDrop
            when Term2.TermType.TABLE_LIST   then RDBTableList
            when Term2.TermType.FUNCALL      then RDBFuncall
            when Term2.TermType.BRANCH       then RDBBranch
            when Term2.TermType.ANY          then RDBAny
            when Term2.TermType.ALL          then RDBAll
            when Term2.TermType.FOREACH      then RDBForEach
            when Term2.TermType.FUNC         then RDBFunc
            else throw new RqlRuntimeError "Unknown Term Type"

        args = (@buildTerm arg for arg in term.argsArray())

        optArgs = {}
        for pair in term.optargsArray()
            optArgs[pair.getKey()] = @buildTerm pair.getVal()

        return new opClass args, optArgs

    buildDatum: (datum) ->
        switch datum.getType()
            when Datum.DatumType.R_NULL
                new RDBDatum new RDBPrimitive null
            when Datum.DatumType.R_BOOL
                new RDBDatum new RDBPrimitive datum.getRBool()
            when Datum.DatumType.R_NUM
                new RDBDatum new RDBPrimitive datum.getRNum()
            when Datum.DatumType.R_STR
                new RDBDatum new RDBPrimitive datum.getRStr()
            when Datum.DatumType.R_ARRAY
                new RDBDatum new RDBArray datum.rArrayArray()
            when Datum.DatumType.R_OBJECT
                obj = new RDBObject()
                for pair in datum.rObjectArray()
                    obj[pair.getKey()] = @buildDatum pair.getVal()
                new RDBDatum obj
            else throw new RqlRuntimeError "Unknown Datum Type"
    
    deconstructDatum: (datum) ->
        res = new Datum
        switch datum.typeOf()
            when RDBType.ARRAY
                res.setType Datum.DatumType.R_ARRAY
                (res.addRArray @deconstructDatum v for v in datum.asArray())
            when RDBType.BOOLEAN
                res.setType Datum.DatumType.R_BOOL
                res.setRBool datum.asJSON()
            when RDBType.NULL
                res.setType Datum.DatumType.R_NULL
            when RDBType.NUMBER
                res.setType Datum.DatumType.R_NUM
                res.setRNum datum.asJSON()
            when RDBType.OBJECT
                res.setType Datum.DatumType.R_OBJECT
                for own k,v of datum
                    pair = new Datum.AssocPair
                    pair.setKey k
                    pair.setVal @deconstructDatum v
                    res.addRObject pair
            when RDBType.STRING
                res.setType Datum.DatumType.R_STR
                res.setRStr datum.asJSON()
        return res
