net = require('net')
events = require('events')

util = require('./util')
err = require('./errors')
cursors = require('./cursor')
pb = require('./protobuf')

r = require('./ast')

# Import some names to this namespace for convienience
ar = util.ar
varar = util.varar
aropt = util.aropt

deconstructDatum = (datum) ->
    pb.DatumTypeSwitch(datum, {
        "R_NULL": =>
            null
       ,"R_BOOL": =>
            datum.r_bool
       ,"R_NUM": =>
            datum.r_num
       ,"R_STR": =>
            datum.r_str
       ,"R_ARRAY": =>
            deconstructDatum dt for dt in datum.r_array
       ,"R_OBJECT": =>
            obj = {}
            for pair in datum.r_object
                obj[pair.key] = deconstructDatum(pair.val)
            obj
        },
            => throw new err.RqlDriverError "Unknown Datum type"
        )

class Connection extends events.EventEmitter
    DEFAULT_HOST: 'localhost'
    DEFAULT_PORT: 28015
    DEFAULT_AUTH_KEY: ''
    DEFAULT_TIMEOUT: 20 # In seconds

    constructor: (host, callback) ->
        if typeof host is 'undefined'
            host = {}
        else if typeof host is 'string'
            host = {host: host}

        @host = host.host || @DEFAULT_HOST
        @port = host.port || @DEFAULT_PORT
        @db = host.db # left undefined if this is not set
        @authKey = host.authKey || @DEFAULT_AUTH_KEY
        @timeout = host.timeout || @DEFAULT_TIMEOUT

        @outstandingCallbacks = {}
        @nextToken = 1
        @open = false

        @buffer = new ArrayBuffer 0

        @_events = @_events || {}

        errCallback = (e) =>
            @removeListener 'connect', conCallback
            if e instanceof err.RqlDriverError
                callback e
            else
                callback new err.RqlDriverError "Could not connect to #{@host}:#{@port}."
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

            responseBuffer = @buffer.slice(4, responseLength + 4)
            response = pb.ParseResponse(responseBuffer)
            @_processResponse response

            # For some reason, Arraybuffer.slice is not in my version of node
            @buffer = @buffer.slice(4 + responseLength)

    mkAtom = (response) -> deconstructDatum response.response[0]

    mkSeq = (response) -> (deconstructDatum res for res in response.response)

    mkErr = (ErrClass, response, root) ->
        msg = mkAtom response
        bt = for frame in response.backtrace.frames
                if frame.type is "POS"
                    parseInt frame.pos
                else
                    frame.opt
        new ErrClass msg, root, bt

    _delQuery: (token) ->
        # This query is done, delete this cursor
        delete @outstandingCallbacks[token]

        if Object.keys(@outstandingCallbacks).length < 1 and not @open
            @cancel()

    _processResponse: (response) ->
        token = response.token
        if @outstandingCallbacks[token]?
            {cb:cb, root:root, cursor: cursor} = @outstandingCallbacks[token]
            if cursor?
                pb.ResponseTypeSwitch(response, {
                     "SUCCESS_PARTIAL": =>
                        cursor._addData mkSeq response
                    ,"SUCCESS_SEQUENCE": =>
                        cursor._endData mkSeq response
                        @_delQuery(token)
                },
                    => cb new err.RqlDriverError "Unknown response type"
                )
            else if cb?
                # Behavior varies considerably based on response type
                pb.ResponseTypeSwitch(response, {
                    "COMPILE_ERROR": =>
                        cb mkErr(err.RqlCompileError, response, root)
                        @_delQuery(token)
                   ,"CLIENT_ERROR": =>
                        cb mkErr(err.RqlClientError, response, root)
                        @_delQuery(token)
                   ,"RUNTIME_ERROR": =>
                        cb mkErr(err.RqlRuntimeError, response, root)
                        @_delQuery(token)
                   ,"SUCCESS_ATOM": =>
                        response = mkAtom response
                        if Array.isArray response
                            response = cursors.makeIterable response
                        cb null, response
                        @_delQuery(token)
                   ,"SUCCESS_PARTIAL": =>
                        cursor = new cursors.Cursor @, token
                        @outstandingCallbacks[token].cursor = cursor
                        cb null, cursor._addData(mkSeq response)
                   ,"SUCCESS_SEQUENCE": =>
                        cursor = new cursors.Cursor @, token
                        @_delQuery(token)
                        cb null, cursor._endData(mkSeq response)
                },
                    => cb new err.RqlDriverError "Unknown response type"
                )

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
        unless @open then throw new err.RqlDriverError "Connection is closed."

        # Assign token
        token = @nextToken++

        # Construct query
        query = {'global_optargs':[]}
        query.type = "START"
        query.query = term.build()
        query.token = token

        # Set global options
        if @db?
            pair =
                key: 'db'
                val: r.db(@db).build()
            query.global_optargs.push(pair)

        if useOutdated?
            pair =
                key: 'use_outdated'
                val: r.expr(!!useOutdated).build()
            query.global_optargs.push(pair)

        if noreply?
            pair =
                key: 'noreply'
                val: r.expr(!!noreply).build()
            query.global_optargs.push(pair)

        # Save callback
        if (not noreply?) or !noreply
            @outstandingCallbacks[token] = {cb:cb, root:term}

        @_sendQuery(query)

        if noreply? and noreply and typeof(cb) is 'function'
            cb null # There is no error and result is `undefined`

    _continueQuery: (token) ->
        query =
            type: "CONTINUE"
            token: token

        @_sendQuery(query)

    _endQuery: (token) ->
        query =
            type: "STOP"
            token: token

        @_sendQuery(query)
        @_delQuery(token)

    _sendQuery: (query) ->

        # Serialize protobuf
        data = pb.SerializeQuery(query)

        length = data.byteLength
        finalArray = new Uint8Array length + 4
        (new DataView(finalArray.buffer)).setInt32(0, length, true)
        finalArray.set((new Uint8Array(data)), 4)

        @write finalArray.buffer

class TcpConnection extends Connection
    @isAvailable: () -> !(process.browser)

    constructor: (host, callback) ->
        unless TcpConnection.isAvailable()
            throw new err.RqlDriverError "TCP sockets are not available in this environment"

        super(host, callback)

        if @rawSocket?
            @close()

        @rawSocket = net.connect @port, @host
        @rawSocket.setNoDelay()

        handshake_complete = false
        @rawSocket.once 'connect', =>
            # Initialize connection with magic number to validate version
            buf = new ArrayBuffer 8
            buf_view = new DataView buf
            buf_view.setUint32 0, 0x723081e1, true # VersionDummy.Version.V0_2
            buf_view.setUint32 4, @authKey.length, true
            @write buf
            @rawSocket.write @authKey, 'ascii'

            # Now we have to wait for a response from the server
            # acknowledging the new connection
            handshake_callback = (buf) =>
                arr = util.toArrayBuffer(buf)

                @buffer = bufferConcat @buffer, arr
                for b,i in new Uint8Array @buffer
                    if b is 0
                        @rawSocket.removeListener('data', handshake_callback)

                        status_buf = @buffer.slice(0, i)
                        @buffer = @buffer.slice(i + 1)
                        status_str = String.fromCharCode.apply(null, new Uint8Array status_buf)

                        handshake_complete = true
                        if status_str == "SUCCESS"
                            # We're good, finish setting up the connection
                            @rawSocket.on 'data', (buf) =>
                                @_data(util.toArrayBuffer(buf))

                            @emit 'connect'
                            return
                        else
                            @emit 'error', new err.RqlDriverError "Server dropped connection with message: \"" + status_str.trim() + "\""
                            return

            @rawSocket.on 'data', handshake_callback

        @rawSocket.on 'error', (args...) => @emit 'error', args...

        @rawSocket.on 'close', => @open = false; @emit 'close'

        setTimeout( (()=>
            if not handshake_complete
                @rawSocket.destroy()
                @emit 'error', new err.RqlDriverError "Handshake timedout"
        ), @timeout*1000)


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
            throw new err.RqlDriverError "XMLHttpRequest is not available in this environment"

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
                    @emit 'error', new err.RqlDriverError "XHR error, http status #{xhr.status}."
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

bufferConcat = (buf1, buf2) ->
    view = new Uint8Array (buf1.byteLength + buf2.byteLength)
    view.set new Uint8Array(buf1), 0
    view.set new Uint8Array(buf2), buf1.byteLength
    view.buffer

# The only exported function of this module
module.exports.connect = ar (host, callback) ->
    # Host must be a string or an object
    unless typeof(host) is 'string' or typeof(host) is 'object'
        throw new err.RqlDriverError "First argument to `connect` must be a string giving the "+
                                 "host to `connect` to or an object giving `host` and `port`."

    # Callback must be a function
    unless typeof(callback) is 'function'
        throw new err.RqlDriverError "Second argument to `connect` must be a callback to invoke with "+
                                 "either an error or the successfully established connection."

    if TcpConnection.isAvailable()
        new TcpConnection host, callback
    else if HttpConnection.isAvailable()
        new HttpConnection host, callback
    else
        throw new err.RqlDriverError "Neither TCP nor HTTP avaiable in this environment"
    return
