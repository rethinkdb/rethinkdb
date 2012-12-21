goog.provide('rethinkdb.PbServer')

goog.require('rethinkdb.Errors')
goog.require('rethinkdb.RDBDatabase')
goog.require('rethinkdb.Universe')
goog.require('rethinkdb.RDBType')
goog.require('rethinkdb.AST')
goog.require('goog.proto2.WireFormatSerializer')
goog.require('Query2')

class RDBPbServer
    constructor: ->
        @serializer = new goog.proto2.WireFormatSerializer()

        # Universe of databases

        # global universe? uhoh?
        global.universe = new RDBUniverse()
        
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

        try
            result = ast.eval()
            if result instanceof RDBSequence
                result.setType Response2.ResponseType.SUCCESS_SEQUENCE
            else
                result.setType Response2.ResponseType.SUCCESS_ATOM
            for datum in result
                result.addResponse @deconstruct datum
        catch err
            unless err instanceof RDBError then throw err
            response.setType Response2.ResponseType.RUNTIME_ERROR

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
            return @evalDatum term.getDatum()

        # Else builtin function
        opClass = switch term.getType()
            when Term2.TermType.MAKE_ARRAY   then MakeArray
            when Term2.TermType.MAKE_OBJ     then MakeObj
            when Term2.TermType.VAR          then throw new ServerError "Not Implemented"
            when Term2.TermType.JAVASCRIPT   then JavaScript
            when Term2.TermType.ERROR         then UserError
            when Term2.TermType.IMPLICIT_VAR then throw new ServerError "Not Implemented"
            when Term2.TermType.DB              then DBRef
            when Term2.TermType.TABLE         then TableRef
            when Term2.TermType.GET             then GetByKey
            when Term2.TermType.EQ             then Eq
            when Term2.TermType.NE             then Ne
            when Term2.TermType.LT             then Lt
            when Term2.TermType.LE             then Le
            when Term2.TermType.GT             then Gt
            when Term2.TermType.GE             then Ge
            when Term2.TermType.NOT             then throw new ServerError "Not Implemented"
            when Term2.TermType.ADD             then throw new ServerError "Not Implemented"
            when Term2.TermType.SUB             then throw new ServerError "Not Implemented"
            when Term2.TermType.MUL             then throw new ServerError "Not Implemented"
            when Term2.TermType.DIV             then throw new ServerError "Not Implemented"
            when Term2.TermType.MOD             then throw new ServerError "Not Implemented"
            else throw new RuntimeError "Unknown Term Type"

        args = term.argsArray()
        optArgs = {}
        for pair in term.optargsArray()
            optArgs[pair.getKey()] = @evalTerm pair.getVal()

        return new opClass args, optArgs

    buildDatum: (datum) ->
        switch datum.getType()
            when Datum.DatumType.R_NULL
                new RDBPrimitive null
            when Datum.DatumType.R_BOOL
                new RDBPrimitive datum.getRBool()
            when Datum.DatumType.R_NUM
                new RDBPrimitive datum.getNum()
            when Datum.DatumType.R_STR
                new RDBPrimitive datum.getStr()
            when Datum.DatumType.R_ARRAY
                new RDBArray datum.rArrayArray()
            when Datum.DatumType.R_OBJECT
                obj = new RDBObject()
                for pair in datum.rObjectArray()
                    obj[pair.getKey()] = @evalDatum pair.getVal()
                obj
            else throw new RuntimeError "Unknown Datum Type"
    
    deconstructDatum: (datum) ->
        res = new Datum
        switch datum.typeOf()
            when RDBType.ARRAY
                res.setType Datum.DatumType.R_ARRAY
                (res.addRArray @deconstructDatum v for v in datum.asArray())
            when RDBType.BOOLEAN
                res.setType Datum.DatumType.R_BOOLEAN
                res.setRBool datum.asJSON()
            when RDBType.NULL
                res.setType Datum.DatumType.R_NULL
            when RDBType.NUMBER
                res.setType Datum.DatumType.R_NUMBER
                res.setRNum datum.asJSON()
            when RDBType.OBJECT
                res.setType Datum.DatumType.R_OBJECT
                for own k,v of datum
                    pair = new AssocPair
                    pair.setKey k
                    pair.setVal @deconstructDatum v
                    res.addRObject pair
            when RDBType.STRING
                res.setType Datum.DatumType.R_STRING
                res.setRStr datum.asJSON()
        return res
