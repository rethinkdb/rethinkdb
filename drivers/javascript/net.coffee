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
deconstructDatum = util.deconstructDatum
mkAtom = util.mkAtom
mkErr = util.mkErr
mkSeq = util.mkSeq

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
                callback new err.RqlDriverError "Could not connect to #{@host}:#{@port}."
        @once 'error', errCallback

        conCallback = =>
            @removeListener 'error', errCallback
            @open = true
            callback null, @
        @once 'connect', conCallback


    _data: (buf) ->
        # Buffer data, execute return results if need be
        @buffer = Buffer.concat([@buffer, buf])

        while @buffer.length >= 4
            responseLength = @buffer.readUInt32LE(0)
            unless @buffer.length >= (4 + responseLength)
                break

            responseBuffer = @buffer.slice(4, responseLength + 4)
            response = pb.ParseResponse(responseBuffer)
            @_processResponse response

            @buffer = @buffer.slice(4 + responseLength)

    _delQuery: (token) ->
        # This query is done, delete this cursor
        delete @outstandingCallbacks[token]

        if Object.keys(@outstandingCallbacks).length < 1 and not @open
            @cancel()

    _processResponse: (response) ->
        token = response.token
        profile = response.profile
        if profile?
            profile = deconstructDatum(profile, {})
        if @outstandingCallbacks[token]?
            {cb:cb, root:root, cursor: cursor, opts: opts} = @outstandingCallbacks[token]
            if cursor?
                cursor._addResponse(response)

                if cursor._endFlag && cursor._outstandingRequests is 0
                    @_delQuery(token)

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
                        response = mkAtom response, opts
                        if Array.isArray response
                            response = cursors.makeIterable response
                        if profile?
                            response = {profile: profile, value: response}
                        cb null, response
                        @_delQuery(token)
                   ,"SUCCESS_PARTIAL": =>
                        cursor = new cursors.Cursor @, token, opts, root
                        @outstandingCallbacks[token].cursor = cursor
                        if profile?
                            cb null, {profile: profile, value: cursor._addResponse(response)}
                        else
                            cb null, cursor._addResponse(response)
                   ,"SUCCESS_SEQUENCE": =>
                        cursor = new cursors.Cursor @, token, opts, root
                        @_delQuery(token)
                        if profile?
                            cb null, {profile: profile, value: cursor._addResponse(response)}
                        else
                            cb null, cursor._addResponse(response)
                   ,"WAIT_COMPLETE": =>
                        @_delQuery(token)
                        cb null, null
                },
                    => cb new err.RqlDriverError "Unknown response type"
                )
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
        query.type = "NOREPLY_WAIT"
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

        if opts.useOutdated?
            pair =
                key: 'use_outdated'
                val: r.expr(!!opts.useOutdated).build()
            query.global_optargs.push(pair)

        if opts.noreply?
            pair =
                key: 'noreply'
                val: r.expr(!!opts.noreply).build()
            query.global_optargs.push(pair)

        if opts.profile?
            pair =
                key: 'profile'
                val: r.expr(!!opts.profile).build()
            query.global_optargs.push(pair)

        # Save callback
        if (not opts.noreply?) or !opts.noreply
            @outstandingCallbacks[token] = {cb:cb, root:term, opts:opts}

        @_sendQuery(query)

        if opts.noreply? and opts.noreply and typeof(cb) is 'function'
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

    _sendQuery: (query) ->
        query.accepts_r_json = true

        # Serialize protobuf
        data = pb.SerializeQuery(query)

        # Prepend length
        totalBuf = new Buffer(data.length + 4)
        totalBuf.writeUInt32LE(data.length, 0)

        # Why loop and not just use Buffer.concat? Good question,
        # The browserify implementation of Buffer.concat seems to
        # be broken.
        i = 0
        while i < data.length
            totalBuf.set(i+4, data.get(i))
            i++

        @write totalBuf

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
            buf = new Buffer(8)
            buf.writeUInt32LE(0x723081e1, 0) # VersionDummy.Version.V0_2
            buf.writeUInt32LE(@authKey.length, 4)
            @write buf
            @rawSocket.write @authKey, 'ascii'

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

    write: (chunk) -> @rawSocket.write chunk

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
                # Convert response from ArrayBuffer to node buffer

                buf = new Buffer(b for b in (new Uint8Array(xhr.response)))
                @_data(buf)

        # Convert the chunk from node buffer to ArrayBuffer
        array = new ArrayBuffer(chunk.length)
        view = new Uint8Array(array)
        i = 0
        while i < chunk.length
            view[i] = chunk.get(i)
            i++

        xhr.send array

# The only exported function of this module
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
