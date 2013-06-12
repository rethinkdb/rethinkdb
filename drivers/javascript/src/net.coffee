goog.provide("rethinkdb.net")

goog.require("rethinkdb.base")
goog.require("rethinkdb.cursor")
goog.require("VersionDummy")
goog.require("Query")
goog.require("goog.proto2.WireFormatSerializer")

# Eventually when we ditch Closure we can actually use the node
# event emitter library. For now it's simple enough that we'll
# emulate the interface.
# var events = require('events')

class Connection
    DEFAULT_HOST: 'localhost'
    DEFAULT_PORT: 28015
    DEFAULT_AUTH_KEY: ''

    constructor: (host, callback) ->
        if typeof host is 'undefined'
            host = {}
        else if typeof host is 'string'
            host = {host: host}

        @host = host.host || @DEFAULT_HOST
        @port = host.port || @DEFAULT_PORT
        @db = host.db # left undefined if this is not set
        @auth_key = host.auth_key || @DEFAULT_AUTH_KEY

        @outstandingCallbacks = {}
        @nextToken = 1
        @open = false

        @buffer = new ArrayBuffer 0

        @_events = @_events || {}

        errCallback = (e) =>
            @removeListener 'connect', conCallback
            callback new RqlDriverError "Could not connect to #{@host}:#{@port}."
        @once 'error', errCallback

        conCallback = =>
            @removeListener 'error', errCallback
            @open = true
            callback null, @
        @once 'connect', conCallback


    _data: (buf) ->
        # Buffer data, execute return results if need be
        @buffer = bufferConcat @buffer, buf

        while @buffer.byteLength >= 4
            responseLength = (new DataView @buffer).getUint32 0, true
            unless @buffer.byteLength >= (4 + responseLength)
                break

            responseArray = new Uint8Array @buffer, 4, responseLength
            deserializer = new goog.proto2.WireFormatSerializer
            response = deserializer.deserialize Response.getDescriptor(), responseArray
            @_processResponse response

            # For some reason, Arraybuffer.slice is not in my version of node
            @buffer = bufferSlice @buffer, (4 + responseLength)

    mkAtom = (response) -> DatumTerm::deconstruct response.getResponse 0

    mkSeq = (response) -> (DatumTerm::deconstruct res for res in response.responseArray())

    mkErr = (ErrClass, response, root) ->
        msg = mkAtom response
        bt = for frame in response.getBacktrace().framesArray()
                if frame.getType() is Frame.FrameType.POS
                    parseInt frame.getPos()
                else
                    frame.getOpt()
        new ErrClass msg, root, bt

    _delQuery: (token) ->
        # This query is done, delete this cursor
        delete @outstandingCallbacks[token]

        if Object.keys(@outstandingCallbacks).length < 1 and not @open
            @cancel()

    _processResponse: (response) ->
        token = response.getToken()
        if @outstandingCallbacks[token]?
            {cb:cb, root:root, cursor: cursor} = @outstandingCallbacks[token]
            if cursor?
                if response.getType() is Response.ResponseType.SUCCESS_PARTIAL
                    cursor._addData mkSeq response
                else if response.getType() is Response.ResponseType.SUCCESS_SEQUENCE
                    cursor._endData mkSeq response
                    @_delQuery(token)
            else if cb?
                # Behavior varies considerably based on response type
                if response.getType() is Response.ResponseType.COMPILE_ERROR
                    cb mkErr(RqlCompileError, response, root)
                    @_delQuery(token)
                else if response.getType() is Response.ResponseType.CLIENT_ERROR
                    cb mkErr(RqlClientError, response, root)
                    @_delQuery(token)
                else if response.getType() is Response.ResponseType.RUNTIME_ERROR
                    cb mkErr(RqlRuntimeError, response, root)
                    @_delQuery(token)
                else if response.getType() is Response.ResponseType.SUCCESS_ATOM
                    response = mkAtom response
                    if goog.isArray response
                        response = ArrayResult::makeIterable response
                    cb null, response
                    @_delQuery(token)
                else if response.getType() is Response.ResponseType.SUCCESS_PARTIAL
                    cursor = new Cursor @, token
                    @outstandingCallbacks[token].cursor = cursor
                    cb null, cursor._addData(mkSeq response)
                else if response.getType() is Response.ResponseType.SUCCESS_SEQUENCE
                    cursor = new Cursor @, token
                    @_delQuery(token)
                    cb null, cursor._endData(mkSeq response)
                else
                    cb new RqlDriverError "Unknown response type"

    close: ar () ->
        @open = false

    cancel: ar () ->
        @outstandingCallbacks = {}

    reconnect: ar (callback) ->
        cb = => @constructor.call(@, {host:@host, port:@port}, callback)
        setTimeout(cb, 0)

    use: ar (db) ->
        @db = db

    _start: (term, cb, useOutdated, noreply) ->
        unless @open then throw new RqlDriverError "Connection is closed."

        # Assign token
        token = ''+@nextToken
        @nextToken++

        # Construct query
        query = new Query
        query.setType Query.QueryType.START
        query.setQuery term.build()
        query.setToken token

        # Set global options
        if @db?
            pair = new Query.AssocPair()
            pair.setKey('db')
            pair.setVal((new Db {}, @db).build())
            query.addGlobalOptargs(pair)

        if useOutdated?
            pair = new Query.AssocPair()
            pair.setKey('use_outdated')
            pair.setVal((new DatumTerm (!!useOutdated)).build())
            query.addGlobalOptargs(pair)

        if noreply?
            pair = new Query.AssocPair()
            pair.setKey('noreply')
            pair.setVal((new DatumTerm (!!noreply)).build())
            query.addGlobalOptargs(pair)

        # Save callback
        if (not noreply?) or !noreply
            @outstandingCallbacks[token] = {cb:cb, root:term}

        @_sendQuery(query)

        if noreply? and noreply and typeof(cb) is 'function'
                cb null # There is no error and result is `undefined`

    _continueQuery: (token) ->
        query = new Query
        query.setType Query.QueryType.CONTINUE
        query.setToken token

        @_sendQuery(query)

    _endQuery: (token) ->
        query = new Query
        query.setType Query.QueryType.STOP
        query.setToken token

        @_sendQuery(query)
        @_delQuery(token)

    _sendQuery: (query) ->

        # Serialize protobuf
        serializer = new goog.proto2.WireFormatSerializer
        data = serializer.serialize query

        length = data.byteLength
        finalArray = new Uint8Array length + 4
        (new DataView(finalArray.buffer)).setInt32(0, length, true)
        finalArray.set data, 4

        @write finalArray.buffer

    ## Emulate the event emitter interface


    addListener: (event, listener) ->
        unless @_events[event]?
            @_events[event] = []
        @_events[event].push(listener)

    on: (event, listener) -> @addListener(event, listener)

    once: (event, listener) ->
        listener._once = true
        @addListener(event, listener)

    removeListener: (event, listener) ->
        if @_events[event]?
            @_events[event] = @_events[event].filter (lst) -> (lst isnt listener)

    removeAllListeners: (event) -> delete @_events[event]

    listeners: (event) -> @_events[event]

    emit: (event, args...) ->
        if @_events[event]?
            listeners = @_events[event].concat()
            for lst in listeners
                if lst._once?
                    @removeListener(event, lst)
                lst.apply(null, args)

class TcpConnection extends Connection
    @isAvailable: -> typeof require isnt 'undefined' and require('net')

    constructor: (host, callback) ->
        unless TcpConnection.isAvailable()
            throw new RqlDriverError "TCP sockets are not available in this environment"

        super(host, callback)

        if @rawSocket?
            @close()

        net = require('net')
        @rawSocket = net.connect @port, @host
        @rawSocket.setNoDelay()

        @rawSocket.once 'connect', =>
            # Initialize connection with magic number to validate version
            buf = new ArrayBuffer 8
            buf_view = new DataView buf
            buf_view.setUint32 0, VersionDummy.Version.V0_2, true
            buf_view.setUint32 4, @auth_key.length, true
            @write buf
            @rawSocket.write @auth_key, 'ascii'

            # Now we have to wait for a response from the server
            # acknowledging the new connection
            handshake_callback = (buf) =>
                arr = toArrayBuffer(buf)

                @buffer = bufferConcat @buffer, arr
                for b,i in new Uint8Array @buffer
                    if b is 0
                        @rawSocket.removeListener('data', handshake_callback)

                        status_buf = bufferSlice(@buffer, 0, i)
                        @buffer = bufferSlice(@buffer, i)
                        status_str = String.fromCharCode.apply(null, new Uint8Array status_buf)

                        if status_str == "SUCCESS"
                            # We're good, finish setting up the connection
                            @rawSocket.on 'data', (buf) =>
                                @_data(toArrayBuffer(buf))

                            @emit 'connect'
                            return
                        else
                            @emit 'error', new RqlDriverError status_str
                            return

            @rawSocket.on 'data', handshake_callback

        @rawSocket.on 'error', (args...) => @emit 'error', args...

        @rawSocket.on 'close', => @open = false; @emit 'close'

    close: () ->
        super()
        @rawSocket.end()

    cancel: () ->
        @rawSocket.destroy()
        super()

    write: (chunk) ->
        # Alas we must convert to a node buffer
        buf = new Buffer chunk.byteLength
        for byte,i in (new Uint8Array chunk)
            buf[i] = byte

        @rawSocket.write buf

class HttpConnection extends Connection
    DEFAULT_PROTOCOL: 'http'

    @isAvailable: -> typeof XMLHttpRequest isnt "undefined"
    constructor: (host, callback) ->
        unless HttpConnection.isAvailable()
            throw new RqlDriverError "XMLHttpRequest is not available in this environment"

        super(host, callback)

        protocol = if host.protocol is 'https' then 'https' else @DEFAULT_PROTOCOL
        url = "#{protocol}://#{@host}:#{@port}/ajax/reql/"
        xhr = new XMLHttpRequest
        xhr.open("GET", url+"open-new-connection", true)
        xhr.responseType = "arraybuffer"

        xhr.onreadystatechange = (e) =>
            if xhr.readyState is 4
                if xhr.status is 200
                    @_url = url
                    @_connId = (new DataView xhr.response).getInt32(0, true)
                    @emit 'connect'
                else
                    @emit 'error', new RqlDriverError "XHR error, http status #{xhr.status}."
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
        @emit 'connect'

    write: (chunk) -> @_data(@_embeddedServer.execute(chunk))

rethinkdb.connect = ar (host, callback) ->
    # Host must be a string or an object
    unless typeof(host) is 'string' or typeof(host) is 'object'
        throw new RqlDriverError "First argument to `connect` must be a string giving the "+
                                 "host to `connect` to or an object giving `host` and `port`."

    # Callback must be a function
    unless typeof(callback) is 'function'
        throw new RqlDriverError "Second argument to `connect` must be a callback to invoke with "+
                                 "either an error or the successfully established connection."

    if TcpConnection.isAvailable()
        new TcpConnection host, callback
    else if HttpConnection.isAvailable()
        new HttpConnection host, callback
    else
        throw new RqlDriverError "Neither TCP nor HTTP avaiable in this environment"
    return

rethinkdb.embeddedConnect = ar (callback) ->
    unless callback? then callback = (->)
    unless EmbeddedConnection.isAvailable()
        throw new RqlDriverError "Embedded connection not available in this environment"
    new EmbeddedConnection new RDBPbServer, callback

bufferConcat = (buf1, buf2) ->
    view = new Uint8Array (buf1.byteLength + buf2.byteLength)
    view.set new Uint8Array(buf1), 0
    view.set new Uint8Array(buf2), buf1.byteLength
    view.buffer

bufferSlice = (buffer, offset, end) ->
    return buffer.slice(offset, end)

toArrayBuffer = (node_buffer) ->
    # Convert from node buffer to array buffer
    arr = new Uint8Array new ArrayBuffer node_buffer.length
    for byte,i in node_buffer
        arr[i] = byte
    return arr.buffer
