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

        @outstandingCallbacks = {}
        @nextToken = 1
        @open = false

        @buffer = new ArrayBuffer 0

        @_connect = =>
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
        {cb:cb, root:root} = @outstandingCallbacks[token]
        if cb
            atom = DatumTerm.deconstruct response.getResponse 0
            sequ = (DatumTerm.deconstruct res for res in response.responseArray())
            cursor = new Cursor @, token
            
            msg = DatumTerm.deconstruct response.getResponse 0
            bt = for frame in response.backtraceArray()
                    if frame.getType() is Response2.Frame.FrameType.POS
                        parseInt frame.getPos()
                    else
                        frame.getOpt()
            err = new RuntimeError msg, root, bt
                                    
            if response.getType() is Response2.ResponseType.SUCCESS_PARTIAL
                cb null, cursor._addData(sequ)
            else
                switch response.getType()
                    when Response2.ResponseType.COMPILE_ERROR
                        cb err
                    when Response2.ResponseType.CLIENT_ERROR
                        cb err
                    when Response2.ResponseType.RUNTIME_ERROR
                        cb err
                    when Response2.ResponseType.SUCCESS_ATOM
                        cb null, atom
                    when Response2.ResponseType.SUCCESS_SEQUENCE
                        cb null, cursor._endData(sequ)
                    else
                        cb new DriverError "Unknown response type"
                # This query is done, delete this cursor
                delete @outstandingCallbacks[token]

                if (k for own k of @outstandingCallbacks).length < 1 and not @open
                    @cancel()
        else
            @_error new DriverError "Unknown token in response"

    close: ->
        @open = false

    cancel: ->
        @outstandingCallbacks = {}
        @close()

    _start: (term, cb) ->
        unless @open then throw DriverError "Connection closed"

        # Assign token
        token = ''+@nextToken
        @nextToken++

        # Construct query
        query = new Query2
        query.setType Query2.QueryType.START
        query.setQuery term.build()
        query.setToken token

        # Save callback
        @outstandingCallbacks[token] = {cb:cb, root:term}

        @_sendQuery(query)
        
    _continue: (token) ->
        query = new Query2
        query.setType Query2.QueryType.CONTINUE
        query.setToken token

        @_sendQuery(query)

    _end: (token) ->
        query = new Query2
        query.setType Query2.QueryType.END
        query.setToken token

        @_sendQuery(query)

    _sendQuery: (query) ->

        # Serialize protobuf
        serializer = new goog.proto2.WireFormatSerializer
        data = serializer.serialize query

        length = data.byteLength
        finalArray = new Uint8Array length + 4
        (new DataView(finalArray.buffer)).setInt32(0, length, true)
        finalArray.set data, 4

        @write finalArray.buffer

class TcpConnection extends Connection
    @isAvailable: -> typeof require isnt 'undefined' and require('net')

    constructor: (host, callback) ->
        unless TcpConnection.isAvailable()
            throw new DriverError "TCP sockets are not available in this environment"

        super(host, callback)

        net = require('net')
        @rawSocket = net.connect @port, @host
        @rawSocket.setNoDelay()

        @rawSocket.on 'connect', =>
            # Initialize connection with magic number to validate version
            buf = new ArrayBuffer 4
            (new DataView buf).setUint32 0, VersionDummy.Version.V0_1, true
            @write buf
            @_connect()

        @rawSocket.on 'error', => @_error()

        @rawSocket.on 'end', => @_end()

        @rawSocket.on 'data', (buf) =>
            # Convert from node buffer to array buffer
            arr = new Uint8Array new ArrayBuffer buf.length
            for byte,i in buf
                arr[i] = byte
            @_data(arr.buffer)

    cancel: ->
        @rawSocket.destroy()
        super()

    write: (chunk) ->
        # Alas we must convert to a node buffer
        buf = new Buffer chunk.byteLength
        for byte,i in (new Uint8Array chunk)
            buf[i] = byte

        @rawSocket.write buf

class HttpConnection extends Connection
    @isAvailable: -> typeof XMLHttpRequest isnt "undefined"
    constructor: (host, callback) ->
        unless HttpConnection.isAvailable()
            throw new DriverError "XMLHttpRequest is not available in this environment"

        super(host, callback)

        url = "http://#{@host}:#{@port}/ajax/reql/"
        xhr = new XMLHttpRequest
        xhr.open("GET", url+"open-new-connection", true)
        xhr.responseType = "arraybuffer"

        xhr.onreadystatechange = (e) =>
            if xhr.readyState is 4
                if xhr.status is 200
                    @_url = url
                    @_connId = (new DataView xhr.response).getInt32(0, true)
                    @_connect()
                else
                    @_error()
        xhr.send()

    cancel: ->
        xhr = new XMLHttpRequest
        xhr.open("POST", "#{@_url}close-connection?conn_id=#{@_connId}", true)
        xhr.send()
        @_url = null
        @_connId = null
        super()

    write: (chunk) ->
        xhr = new XMLHttpRequest
        xhr.open("POST", "#{@_url}?conn_id=#{@_connId}", true)
        xhr.responseType = "arraybuffer"

        xhr.onreadystatechange = (e) =>
            if xhr.readyState is 4 and xhr.status is 200
                @_data(xhr.response)
        xhr.send chunk

class EmbeddedConnection extends Connection
    @isAvailable: -> (typeof RDBPbServer isnt "undefined")
    constructor: (embeddedServer, callback) ->
        super({}, callback)
        @_embeddedServer = embeddedServer
        @_connect()

    cancel: -> super()

    write: (chunk) -> @_data(@_embeddedServer.execute(chunk))

rethinkdb.connect = (host, callback) ->
    unless callback? then callback = (->)
    if TcpConnection.isAvailable()
        new TcpConnection host, callback
    else if HttpConnection.isAvailable()
        new HttpConnection host, callback
    else
        throw new DriverError "Neither TCP nor HTTP avaiable in this environment"

rethinkdb.embeddedConnect = (callback) ->
    unless callback? then callback = (->)
    unless EmbeddedConnection.isAvailable()
        throw new DriverError "Embedded connection not available in this environment"
    new EmbeddedConnection new RDBPbServer, callback

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
