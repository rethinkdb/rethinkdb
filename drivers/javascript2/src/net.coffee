goog.provide("rethinkdb.net")

goog.require("rethinkdb.base")
goog.require("rethinkdb.cursor")
goog.require("VersionDummy")
goog.require("Query2")
goog.require("goog.proto2.WireFormatSerializer")

class Connection
    DEFAULT_HOST: 'localhost'
    DEFAULT_PORT: 28016

    constructor: (host, callback) ->
        if typeof host is 'undefined'
            host = {}
        else if typeof host is 'string'
            host = {host: host}

        @host = host.host || @DEFAULT_HOST
        @port = host.port || @DEFAULT_PORT

        @outstandingCursors = {}
        @nextToken = 1
        @open = false

        @buffer = new ArrayBuffer 0

        @_connect = =>
            # Initialize connection with magic number to validate version
            buf = new ArrayBuffer 4
            (new DataView buf).setUint32 0, VersionDummy.Version.V0_1, true
            @write buf
            @open = true
            @_error = => # Clear failed to connect callback
            callback null, @

        @_error = => callback new DriverError "Could not connect to server"

    _data: (buf) ->
        # Buffer data, execute return results if need be
        @buffer = bufferConcat @buffer, buf

        while @buffer.byteLength >= 4
            responseLength = (new DataView @buffer).getUint32 0, true
            responseLength2= (new DataView @buffer).getUint32 0, true
            unless @buffer.byteLength >= (4 + responseLength)
                break

            responseArray = new Uint8Array @buffer, 4, responseLength
            deserializer = new goog.proto2.WireFormatSerializer
            response = deserializer.deserialize Response2.getDescriptor(), responseArray
            @_processResponse response

            # For some reason, Arraybuffer.slice is not in my version of node
            @buffer = bufferSlice @buffer, (4 + responseLength)

    _end: -> @close()

    _processResponse: (response) ->
        token = response.getToken()
        cursor = @outstandingCursors[token]
        if cursor
            atom = DatumTerm.deconstruct response.getResponse 0
            iter = (DatumTerm.deconstruct res for res in response.responseArray())
            
            msg = DatumTerm.deconstruct response.getResponse 0
            bt = for frame in response.backtraceArray()
                    if frame.getType() is Response2.Frame.FrameType.POS
                        parseInt frame.getPos()
                    else
                        frame.getOpt()
            errr = new RuntimeError msg, cursor.root, bt
                                    
            if response.getType() is Response2.ResponseType.SUCCESS_PARTIAL
                cursor.partIter iter
            else
                switch response.getType()
                    when Response2.ResponseType.COMPILE_ERROR
                        cursor.goError errr
                    when Response2.ResponseType.CLIENT_ERROR
                        cursor.goError errr
                    when Response2.ResponseType.RUNTIME_ERROR
                        cursor.goError errr
                    when Response2.ResponseType.SUCCESS_ATOM
                        cursor.goResult DatumTerm.deconstruct response.getResponse 0
                    when Response2.ResponseType.SUCCESS_SEQUENCE
                        cursor.endIter iter
                    else
                        cursor.goError new DriverError "Unknown response type"
                # This query is done, delete this cursor
                delete @outstandingCursors[token]
        else
            @_error new DriverError "Unknown token in response"

    close: ->
        @open = false

    run: (term, cb) ->
        # Assign token
        token = ''+@nextToken
        @nextToken++

        # Construct query
        query = new Query2
        query.setType Query2.QueryType.START
        query.setQuery term.build()
        query.setToken token

        # Save cursor
        @outstandingCursors[token] = new Cursor term, cb

        # Serialize protobuf
        serializer = new goog.proto2.WireFormatSerializer
        data = serializer.serialize query

        length = data.byteLength
        finalArray = new Uint8Array length + 4
        (new DataView(finalArray.buffer)).setInt32(0, length, true)
        finalArray.set data, 4

        @write finalArray

    with: (cb) ->
        @connStack.push @
        cb()
        @connStack.pop

class TcpConnection extends Connection
    @isAvailable: -> typeof require isnt 'undefined' and require('net')

    constructor: (host, callback) ->
        unless TcpConnection.isAvailable()
            throw new DriverError "TCP sockets are not available in this environment"

        super(host, callback)

        net = require('net')
        @rawSocket = net.connect @port, @host
        @rawSocket.setNoDelay()

        @rawSocket.on 'connect', => @_connect()

        @rawSocket.on 'error', => @_error()

        @rawSocket.on 'end', => @_end()

        @rawSocket.on 'data', (buf) =>
            # Convert from node buffer to array buffer
            arr = new Uint8Array new ArrayBuffer buf.length
            for byte,i in buf
                arr[i] = byte
            @_data(arr.buffer)

    close: ->
        @rawSocket.destroy()
        super()

    write: (chunk) ->
        # Alas we must convert to a node buffer
        buf = new Buffer chunk.byteLength
        for byte,i in (new Uint8Array chunk)
            buf[i] = byte

        @rawSocket.write buf

class HttpConnection extends Connection
    @isAvailable: -> return false
    constructor: -> throw new DriverError "Not implemented"

rethinkdb.connect = (host, callback) ->
    unless callback? then callback = (->)
    if TcpConnection.isAvailable()
        new TcpConnection host, callback
    else if HttpConnection.isAvailable()
        new HttpConnection host, callback
    else
        throw new DriverError "Neither TCP nor HTTP avaiable in this environment"

bufferConcat = (buf1, buf2) ->
    view = new Uint8Array (buf1.byteLength + buf2.byteLength)
    view.set new Uint8Array(buf1), 0
    view.set new Uint8Array(buf2), buf1.byteLength
    view.buffer

bufferSlice = (buffer, offset) ->
    if offset > buffer.byteLength then offset = buffer.byteLength
    residual = buffer.byteLength - offset
    res = new Uint8Array residual
    res.set (new Uint8Array buffer, offset)
    res.buffer
