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
            throw new RuntimeError "protocol buffer not of the correct length"

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
            unless err instanceof RDBError then throw err

            response.setType (
                if err instanceof RuntimeError
                    Response2.ResponseType.RUNTIME_ERROR
                else if err instanceof ServerError
                    Response2.ResponseType.RUNTIME_ERROR
                else if err instanceof BadQuery
                    Response2.ResponseType.RUNTIME_ERROR
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
                throw new RuntimeError "Not implemented"
            when Query2.QueryType.STOP
                throw new RuntimeError "Not implemented"

    buildTerm: (term) ->
        if term.getType() is Term2.TermType.DATUM
            return @buildDatum term.getDatum()

        # Else builtin function
        opClass = switch term.getType()
            when Term2.TermType.MAKE_ARRAY   then MakeArray
            when Term2.TermType.MAKE_OBJ     then MakeObj
            when Term2.TermType.VAR          then Var
            when Term2.TermType.JAVASCRIPT   then JavaScript
            when Term2.TermType.ERROR        then UserError
            when Term2.TermType.IMPLICIT_VAR then ImplicitVar
            when Term2.TermType.DB           then DBRef
            when Term2.TermType.TABLE        then TableRef
            when Term2.TermType.GET          then GetByKey
            when Term2.TermType.EQ           then Eq
            when Term2.TermType.NE           then Ne
            when Term2.TermType.LT           then Lt
            when Term2.TermType.LE           then Le
            when Term2.TermType.GT           then Gt
            when Term2.TermType.GE           then Ge
            when Term2.TermType.NOT          then Not
            when Term2.TermType.ADD          then Add
            when Term2.TermType.SUB          then Sub
            when Term2.TermType.MUL          then Mul
            when Term2.TermType.DIV          then Div
            when Term2.TermType.MOD          then Mod
            when Term2.TermType.APPEND       then Append
            when Term2.TermType.SLICE        then Slice
            when Term2.TermType.GETATTR      then GetAttr
            when Term2.TermType.CONTAINS     then Contains
            when Term2.TermType.PLUCK        then Pluck
            when Term2.TermType.WITHOUT      then Without
            when Term2.TermType.MERGE        then Merge
            when Term2.TermType.BETWEEN      then Between
            when Term2.TermType.REDUCE       then Reduce
            when Term2.TermType.MAP          then Map
            when Term2.TermType.FILTER       then Filter
            when Term2.TermType.CONCATMAP    then ConcatMap
            when Term2.TermType.ORDERBY      then OrderBy
            when Term2.TermType.DISTINCT     then Distinct
            when Term2.TermType.COUNT        then Count
            when Term2.TermType.UNION        then Union
            when Term2.TermType.NTH          then Nth
            when Term2.TermType.GROUPED_MAP_REDUCE then GroupedMapReduce
            when Term2.TermType.GROUPBY      then GroupBy
            when Term2.TermType.INNER_JOIN   then InnerJoin
            when Term2.TermType.OUTER_JOIN   then OuterJoin
            when Term2.TermType.EQ_JOIN      then EqJoin
            when Term2.TermType.COERCE       then Coerce
            when Term2.TermType.TYPEOF       then TypeOf
            when Term2.TermType.UPDATE       then Update
            when Term2.TermType.DELETE       then Delete
            when Term2.TermType.REPLACE      then Replace
            when Term2.TermType.INSERT       then Insert
            when Term2.TermType.DB_CREATE    then DbCreate
            when Term2.TermType.DB_DROP      then DbDrop
            when Term2.TermType.DB_LIST      then DbList
            when Term2.TermType.TABLE_CREATE then TableCreate
            when Term2.TermType.TABLE_DROP   then TableDrop
            when Term2.TermType.TABLE_LIST   then TableList
            when Term2.TermType.FUNCALL      then Funcall
            when Term2.TermType.BRANCH       then Branch
            when Term2.TermType.ANY          then Any
            when Term2.TermType.ALL          then All
            when Term2.TermType.FOREACH      then ForEach
            when Term2.TermType.FUNC         then Func
            else throw new RuntimeError "Unknown Term Type"

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
            else throw new RuntimeError "Unknown Datum Type"
    
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
