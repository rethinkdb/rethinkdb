net = require('net')
events = require('events')

util = require('./util')
err = require('./errors')
cursors = require('./cursor')

protodef = require('./proto-def')
protoVersion = protodef.VersionDummy.Version.V0_3
protoProtocol = protodef.VersionDummy.Protocol.JSON
protoQueryType = protodef.Query.QueryType
protoResponseType = protodef.Response.ResponseType

r = require('./ast')

# Import some names to this namespace for convienience
ar = util.ar
varar = util.varar
aropt = util.aropt
mkAtom = util.mkAtom
mkErr = util.mkErr

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

        @buffer = new Buffer(0)

        @_events = @_events || {}

        errCallback = (e) =>
            @removeListener 'connect', conCallback
            if e instanceof err.RqlDriverError
                callback e
            else
                callback new err.RqlDriverError "Could not connect to #{@host}:#{@port}.\n#{e.message}"
        @once 'error', errCallback

        conCallback = =>
            @removeListener 'error', errCallback
            @open = true
            callback null, @
        @once 'connect', conCallback


    _data: (buf) ->
        # Buffer data, execute return results if need be
        @buffer = Buffer.concat([@buffer, buf])

        while @buffer.length >= 12 
            token = @buffer.readUInt32LE(0) + 0x100000000 * @buffer.readUInt32LE(4)

            responseLength = @buffer.readUInt32LE(8)
            unless @buffer.length >= (12 + responseLength)
                break

            responseBuffer = @buffer.slice(12, responseLength + 12)
            response = JSON.parse(responseBuffer)

            @_processResponse response, token
            @buffer = @buffer.slice(12 + responseLength)

    _delQuery: (token) ->
        # This query is done, delete this cursor
        delete @outstandingCallbacks[token]

        if Object.keys(@outstandingCallbacks).length < 1 and not @open
            @cancel()

    _processResponse: (response, token) ->
        profile = response.p
        if @outstandingCallbacks[token]?
            {cb:cb, root:root, cursor: cursor, opts: opts} = @outstandingCallbacks[token]
            if cursor?
                cursor._addResponse(response)

                if cursor._endFlag && cursor._outstandingRequests is 0
                    @_delQuery(token)

            else if cb?
                # Behavior varies considerably based on response type
                switch response.t
                    when protoResponseType.COMPILE_ERROR
                        cb mkErr(err.RqlCompileError, response, root)
                        @_delQuery(token)
                    when protoResponseType.CLIENT_ERROR
                        cb mkErr(err.RqlClientError, response, root)
                        @_delQuery(token)
                    when protoResponseType.RUNTIME_ERROR
                        cb mkErr(err.RqlRuntimeError, response, root)
                        @_delQuery(token)
                    when protoResponseType.SUCCESS_ATOM
                        response = mkAtom response, opts
                        if Array.isArray response
                            response = cursors.makeIterable response
                        if profile?
                            response = {profile: profile, value: response}
                        cb null, response
                        @_delQuery(token)
                    when protoResponseType.SUCCESS_PARTIAL
                        cursor = new cursors.Cursor @, token, opts, root
                        @outstandingCallbacks[token].cursor = cursor
                        if profile?
                            cb null, {profile: profile, value: cursor._addResponse(response)}
                        else
                            cb null, cursor._addResponse(response)
                    when protoResponseType.SUCCESS_SEQUENCE
                        cursor = new cursors.Cursor @, token, opts, root
                        @_delQuery(token)
                        if profile?
                            cb null, {profile: profile, value: cursor._addResponse(response)}
                        else
                            cb null, cursor._addResponse(response)
                    when protoResponseType.WAIT_COMPLETE
                        @_delQuery(token)
                        cb null, null
                    else
                        cb new err.RqlDriverError "Unknown response type"
        else
            # Unexpected token
            @emit 'error', new err.RqlDriverError "Unexpected token #{token}."

    close: (varar 0, 2, (optsOrCallback, callback) ->
        if callback?
            opts = optsOrCallback
            unless Object::toString.call(opts) is '[object Object]'
                throw new err.RqlDriverError "First argument to two-argument `close` must be an object."
            cb = callback
        else if Object::toString.call(optsOrCallback) is '[object Object]'
            opts = optsOrCallback
            cb = null
        else
            opts = {}
            cb = optsOrCallback

        for own key of opts
            unless key in ['noreplyWait']
                throw new err.RqlDriverError "First argument to two-argument `close` must be { noreplyWait: <bool> }."
        unless not cb? or typeof cb is 'function'
            throw new err.RqlDriverError "Final argument to `close` must be a callback function or object."

        wrappedCb = (args...) =>
            @open = false
            if cb?
                cb(args...)

        noreplyWait = ((not opts.noreplyWait?) or opts.noreplyWait) and @open
        if noreplyWait
            @noreplyWait(wrappedCb)
        else
            wrappedCb()
    )

    noreplyWait: ar (callback) ->
        unless typeof callback is 'function'
            throw new err.RqlDriverError "First argument to noreplyWait must be a callback function."
        unless @open
            callback(new err.RqlDriverError "Connection is closed.")
            return

        # Assign token
        token = @nextToken++

        # Construct query
        query = {}
        query.type = protoQueryType.NOREPLY_WAIT
        query.token = token

        # Save callback
        @outstandingCallbacks[token] = {cb:callback, root:null, opts:null}

        @_sendQuery(query)

    cancel: ar () ->
        @outstandingCallbacks = {}

    reconnect: (varar 1, 2, (optsOrCallback, callback) ->
        if callback?
            opts = optsOrCallback
            cb = callback
        else
            opts = {}
            cb = optsOrCallback

        unless typeof cb is 'function'
            throw new err.RqlDriverError "Final argument to `reconnect` must be a callback function."

        closeCb = (err) =>
            if err?
                cb(err)
            else
                constructCb = => @constructor.call(@, {host:@host, port:@port}, cb)
                setTimeout(constructCb, 0)
        @close(opts, closeCb)
    )

    use: ar (db) ->
        @db = db

    _start: (term, cb, opts) ->
        unless @open then throw new err.RqlDriverError "Connection is closed."

        # Assign token
        token = @nextToken++

        # Construct query
        query = {}
        query.global_optargs = {}
        query.type = protoQueryType.START
        query.query = term.build()
        query.token = token
        # Set global options
        if @db?
            query.global_optargs['db'] = r.db(@db).build()

        if opts.useOutdated?
            query.global_optargs['use_outdated'] = r.expr(!!opts.useOutdated).build()

        if opts.noreply?
            query.global_optargs['noreply'] = r.expr(!!opts.noreply).build()

        if opts.profile?
            query.global_optargs['profile'] = r.expr(!!opts.profile).build()

        if opts.durability?
            query.global_optargs['durability'] = r.expr(opts.durability).build()

        if opts.batchConf?
            query.global_optargs['batchConf'] = r.expr(opts.batchConf).build()

        # Save callback
        if (not opts.noreply?) or !opts.noreply
            @outstandingCallbacks[token] = {cb:cb, root:term, opts:opts}

        @_sendQuery(query)

        if opts.noreply? and opts.noreply and typeof(cb) is 'function'
            cb null # There is no error and result is `undefined`

    _continueQuery: (token) ->
        query =
            type: protoQueryType.CONTINUE
            token: token

        @_sendQuery(query)

    _endQuery: (token) ->
        query =
            type: protoQueryType.STOP
            token: token

        @_sendQuery(query)

    _sendQuery: (query) ->
        # Serialize query to JSON
        data = [query.type, query.token]
        if !(query.query is undefined)
            data.push(query.query)
            if query.global_optargs? and Object.keys(query.global_optargs).length > 0
                data.push(query.global_optargs)

        @write new Buffer(JSON.stringify(data))

class TcpConnection extends Connection
    @isAvailable: () -> !(process.browser)

    constructor: (host, callback) ->
        unless TcpConnection.isAvailable()
            throw new err.RqlDriverError "TCP sockets are not available in this environment"

        super(host, callback)

        if @rawSocket?
            @close({noreplyWait: false})

        @rawSocket = net.connect @port, @host
        @rawSocket.setNoDelay()

        timeout = setTimeout( (()=>
            @rawSocket.destroy()
            @emit 'error', new err.RqlDriverError "Handshake timedout"
        ), @timeout*1000)

        @rawSocket.once 'error', => clearTimeout(timeout)

        @rawSocket.once 'connect', =>
            # Initialize connection with magic number to validate version
            buf = new Buffer(4)
            buf.writeUInt32LE(protoVersion, 0)
            @rawSocket.write buf

            buf = new Buffer(@authKey, 'ascii')
            @write buf

            # Send the protocol type that we will be using to communicate with the server
            buf = new Buffer(4)
            buf.writeUInt32LE(protoProtocol, 0)
            @rawSocket.write buf

            # Now we have to wait for a response from the server
            # acknowledging the new connection
            handshake_callback = (buf) =>
                @buffer = Buffer.concat([@buffer, buf])
                for b,i in @buffer
                    if b is 0
                        @rawSocket.removeListener('data', handshake_callback)

                        status_buf = @buffer.slice(0, i)
                        @buffer = @buffer.slice(i + 1)
                        status_str = status_buf.toString()

                        clearTimeout(timeout)
                        if status_str == "SUCCESS"
                            # We're good, finish setting up the connection
                            @rawSocket.on 'data', (buf) => @_data(buf)

                            @emit 'connect'
                            return
                        else
                            @emit 'error', new err.RqlDriverError "Server dropped connection with message: \"" + status_str.trim() + "\""
                            return


            @rawSocket.on 'data', handshake_callback

        @rawSocket.on 'error', (args...) => @emit 'error', args...

        @rawSocket.on 'close', => @open = false; @emit 'close', {noreplyWait: false}

        # In case the raw socket timesout, we close it and re-emit the event for the user
        @rawSocket.on 'timeout', => @open = false; @emit 'timeout'

    close: (varar 0, 2, (optsOrCallback, callback) ->
        if callback?
            opts = optsOrCallback
            cb = callback
        else if Object::toString.call(optsOrCallback) is '[object Object]'
            opts = optsOrCallback
            cb = null
        else
            opts = {}
            cb = optsOrCallback
        unless not cb? or typeof cb is 'function'
            throw new err.RqlDriverError "Final argument to `close` must be a callback function or object."

        wrappedCb = (args...) =>
            @rawSocket.end()
            if cb?
                cb(args...)

        # This would simply be super(opts, wrappedCb), if we were not in the varar
        # anonymous function
        TcpConnection.__super__.close.call(this, opts, wrappedCb)
    )

    cancel: () ->
        @rawSocket.destroy()
        super()

    write: (chunk) ->
        lengthBuffer = new Buffer(4)
        lengthBuffer.writeUInt32LE(chunk.length, 0)
        @rawSocket.write lengthBuffer
        @rawSocket.write chunk

class HttpConnection extends Connection
    DEFAULT_PROTOCOL: 'http'

    @isAvailable: -> typeof XMLHttpRequest isnt "undefined"
    constructor: (host, callback) ->
        unless HttpConnection.isAvailable()
            throw new err.RqlDriverError "XMLHttpRequest is not available in this environment"

        super(host, callback)

        protocol = if host.protocol is 'https' then 'https' else @DEFAULT_PROTOCOL
        url = "#{protocol}://#{@host}:#{@port}#{host.pathname}ajax/reql/"
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

        @xhr = xhr # We allow only one query at a time per HTTP connection

    cancel: ->
        @xhr.abort()
        xhr = new XMLHttpRequest
        xhr.open("POST", "#{@_url}close-connection?conn_id=#{@_connId}", true)
        xhr.send()
        @_url = null
        @_connId = null
        super()

    close: (varar 0, 2, (optsOrCallback, callback) ->
        if callback?
            opts = optsOrCallback
            cb = callback
        else if Object::toString.call(optsOrCallback) is '[object Object]'
            opts = optsOrCallback
            cb = null
        else
            opts = {}
            cb = optsOrCallback
        unless not cb? or typeof cb is 'function'
            throw new err.RqlDriverError "Final argument to `close` must be a callback function or object."

        wrappedCb = (args...) =>
            @cancel()
            if cb?
                cb(args...)

        # This would simply be super(opts, wrappedCb), if we were not in the varar
        # anonymous function
        HttpConnection.__super__.close.call(this, opts, wrappedCb)
    )

    write: (chunk) ->
        xhr = new XMLHttpRequest
        xhr.open("POST", "#{@_url}?conn_id=#{@_connId}", true)
        xhr.responseType = "arraybuffer"

        xhr.onreadystatechange = (e) =>
            if xhr.readyState is 4 and xhr.status is 200
                # Convert response from ArrayBuffer to node buffer

                buf = new Buffer(b for b in (new Uint8Array(xhr.response)))
                @_data(buf)

        # Convert the chunk from node buffer to ArrayBuffer
        array = new ArrayBuffer(chunk.length)
        view = new Uint8Array(array)
        i = 0
        while i < chunk.length
            view[i] = chunk[i]
            i++

        xhr.send array
        @xhr = xhr # We allow only one query at a time per HTTP connection

module.exports.isConnection = (connection) ->
    return connection instanceof Connection

# The main function of this module
module.exports.connect = ar (host, callback) ->
    # Host must be a string or an object
    unless typeof(host) is 'string' or Object::toString.call(host) is '[object Object]'
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
