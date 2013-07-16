# rethinkdb is both the main export object for the module
# and a function that shortcuts `r.expr`.
rethinkdb = (args...) -> rethinkdb.expr(args...)

# Utilities

# Function wrapper that enforces that the function is
# called with the correct number of arguments
ar = (fun) -> (args...) ->
    if args.length isnt fun.length
        throw new RqlDriverError "Expected #{fun.length} argument(s) but found #{args.length}."
    fun.apply(@, args)

# Like ar for variable argument functions. Takes minimum
# and maximum argument parameters.
varar = (min, max, fun) -> (args...) ->
    if (min? and args.length < min) or (max? and args.length > max)
        if min? and not max?
            throw new RqlDriverError "Expected #{min} or more argument(s) but found #{args.length}."
        if max? and not min?
            throw new RqlDriverError "Expected #{max} or fewer argument(s) but found #{args.length}."
        throw new RqlDriverError "Expected between #{min} and #{max} argument(s) but found #{args.length}."
    fun.apply(@, args)

# Like ar but for functions that take an optional options dict as the last argument
aropt = (fun) -> (args...) ->
    expectedPosArgs = fun.length - 1
    perhapsOptDict = args[expectedPosArgs]

    if perhapsOptDict and ((not (perhapsOptDict instanceof Object)) or (perhapsOptDict instanceof TermBase))
        perhapsOptDict = null

    numPosArgs = args.length - (if perhapsOptDict? then 1 else 0)

    if expectedPosArgs isnt numPosArgs
         throw new RqlDriverError "Expected #{expectedPosArgs} argument(s) but found #{numPosArgs}."
    fun.apply(@, args)

# Test if the requested node module is available
testFor = (module) ->
    try
        return typeof require is 'function' and require(module)
    catch e
        return false

funcWrap = (val) ->
    if val is undefined
        # Pass through the undefined value so it's caught by
        # the appropriate undefined checker
        return val

    val = rethinkdb.expr(val)

    ivarScan = (node) ->
        unless node instanceof TermBase then return false
        if node instanceof ImplicitVar then return true
        if (node.args.map ivarScan).some((a)->a) then return true
        if (v for own k,v of node.optargs).map(ivarScan).some((a)->a) then return true
        return false

    if ivarScan(val)
        return new Func {}, (x) -> val

    return val


# Cursors

class IterableResult
    hasNext: -> throw "Abstract Method"
    next: -> throw "Abstract Method"

    each: varar(1, 2, (cb, onFinished) ->
        unless typeof cb is 'function'
            throw new RqlDriverError "First argument to each must be a function."
        if onFinished? and typeof onFinished isnt 'function'
            throw new RqlDriverError "Optional second argument to each must be a function."

        brk = false
        n = =>
            if not brk and @hasNext()
                @next (err, row) =>
                    brk = (cb(err, row) is false)
                    n()
            else if onFinished?
                onFinished()
        n()
    )

    toArray: ar (cb) ->
        unless typeof cb is 'function'
            throw new RqlDriverError "Argument to toArray must be a function."

        arr = []
        if not @hasNext()
            cb null, arr
        @each (err, row) =>
            if err?
                cb err
            else
                arr.push(row)

            if not @hasNext()
                cb null, arr

class Cursor extends IterableResult
    constructor: (conn, token) ->
        @_conn = conn
        @_token = token

        @_chunks = []
        @_endFlag = false
        @_contFlag = false
        @_cont = null
        @_cbQueue = []

    _addChunk: (chunk) ->
        if chunk.length > 0
            @_chunks.push chunk

    _addData: (chunk) ->
        @_addChunk chunk

        @_contFlag = false
        @_promptNext()
        @

    _endData: (chunk) ->
        @_addChunk chunk
        @_endFlag = true

        @_contFlag = true
        @_promptNext()
        @

    _promptNext: ->

        # If there are no more waiting callbacks, just wait until the next event
        while @_cbQueue[0]?

            # If there's no more data let's notify the waiting callback
            if not @hasNext()
                cb = @_cbQueue.shift()
                cb new RqlDriverError "No more rows in the cursor."
            else

                # We haven't processed all the data, let's try to give it to the callback

                # Is there data waiting in our buffer?
                chunk = @_chunks[0]
                if not chunk?
                    # We're out of data for now, let's fetch more (which will prompt us again)
                    @_promptCont()
                    return
                else

                    # After this point there is at least one row in the chunk

                    row = chunk.shift()
                    cb = @_cbQueue.shift()

                    # Did we just empty the chunk?
                    if chunk[0] is undefined
                        # We're done with this chunk, discard it
                        @_chunks.shift()

                    # Finally we can invoke the callback with the row
                    cb null, row

    _promptCont: ->
        # Let's ask the server for more data if we haven't already
        unless @_contFlag
            @_conn._continueQuery(@_token)
            @_contFlag = true


    ## Implement IterableResult

    hasNext: ar () -> !@_endFlag || @_chunks[0]?

    next: ar (cb) ->
        nextCbCheck(cb)
        @_cbQueue.push cb
        @_promptNext()

    close: ar () ->
        unless @_endFlag
            @_conn._endQuery(@_token)

    toString: ar () -> "[object Cursor]"

# Used to wrap array results so they support the same iterable result
# API as cursors.
class ArrayResult extends IterableResult
    hasNext: ar () -> (@__index < @.length)
    next: ar (cb) ->
        nextCbCheck(cb)
        cb(null, @[@__proto__.__index++])

    makeIterable: (response) ->
        for name, method of ArrayResult.prototype
            if name isnt 'constructor'
                response.__proto__[name] = method
        response.__proto__.__index = 0
        response

nextCbCheck = (cb) ->
    unless typeof cb is 'function'
        throw new RqlDriverError "Argument to next must be a function."

# Protobuf

# Initialize message serializer with
desc = require('fs').readFileSync(__dirname + "/ql2.desc")
npb = require('node-protobuf').Protobuf
NodePB = new npb(desc)

# Errors

class RqlDriverError extends Error
    constructor: (msg) ->
        @name = @constructor.name
        @msg = msg
        @message = msg

class RqlServerError extends Error
    constructor: (msg, term, frames) ->
        @name = @constructor.name
        @msg = msg
        @frames = frames[0..]
        @message = "#{msg} in:\n#{RqlQueryPrinter::printQuery(term)}\n#{RqlQueryPrinter::printCarrots(term, frames)}"

class RqlRuntimeError extends RqlServerError

class RqlCompileError extends RqlServerError

class RqlClientError extends RqlServerError

class RqlQueryPrinter
    printQuery: (term) ->
        tree = composeTerm(term)
        joinTree tree

    composeTerm = (term) ->
        args = (composeTerm arg for arg in term.args)
        optargs = {}
        for own key,arg of term.optargs
            optargs[key] = composeTerm(arg)
        term.compose(args, optargs)

    printCarrots: (term, frames) ->
        tree = composeCarrots(term, frames)
        (joinTree tree).replace(/[^\^]/g, ' ')

    composeCarrots = (term, frames) ->
        argNum = frames.shift()
        unless argNum? then argNum = -1

        args = for arg,i in term.args
                    if i == argNum
                        composeCarrots(arg, frames)
                    else
                        composeTerm(arg)

        optargs = {}
        for own key,arg of term.optargs
            optargs[key] = if key == argNum
                             composeCarrots(arg, frames)
                           else
                             composeTerm(arg)
        if argNum != -1
            term.compose(args, optargs)
        else
            carrotify(term.compose(args, optargs))

    carrotify = (tree) -> (joinTree tree).replace(/./g, '^')

    joinTree = (tree) ->
        str = ''
        for term in tree
            if Array.isArray term
                str += joinTree term
            else
                str += term
        return str


# Networking code

EventEmitter = require('events').EventEmitter

class Connection extends EventEmitter
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
            if e instanceof RqlDriverError
                callback e
            else
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
            response = NodePB.Parse(responseArray, "Response")
            @_processResponse response

            # For some reason, Arraybuffer.slice is not in my version of node
            @buffer = @buffer.slice(4 + responseLength)

    mkAtom = (response) -> DatumTerm::deconstruct response.response[0]

    mkSeq = (response) -> (DatumTerm::deconstruct res for res in response.response)

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
                if response.type is "SUCCESS_PARTIAL"
                    cursor._addData mkSeq response
                else if response.type is "SUCCESS_SEQUENCE"
                    cursor._endData mkSeq response
                    @_delQuery(token)
            else if cb?
                # Behavior varies considerably based on response type
                switch response.type
                    when "COMPILE_ERROR"
                        cb mkErr(RqlCompileError, response, root)
                        @_delQuery(token)
                    when "CLIENT_ERROR"
                        cb mkErr(RqlClientError, response, root)
                        @_delQuery(token)
                    when "RUNTIME_ERROR"
                        cb mkErr(RqlRuntimeError, response, root)
                        @_delQuery(token)
                    when "SUCCESS_ATOM"
                        response = mkAtom response
                        if Array.isArray response
                            response = ArrayResult::makeIterable response
                        cb null, response
                        @_delQuery(token)
                    when "SUCCESS_PARTIAL"
                        cursor = new Cursor @, token
                        @outstandingCallbacks[token].cursor = cursor
                        cb null, cursor._addData(mkSeq response)
                    when "SUCCESS_SEQUENCE"
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
                val: (new Db {}, @db).build()
            query.global_optargs.push(pair)

        if useOutdated?
            pair =
                key: 'use_outdated'
                val: (new DatumTerm (!!useOutdated)).build()
            query.global_optargs.push(pair)

        if noreply?
            pair =
                key: 'noreply'
                val: (new DatumTerm (!!noreply)).build()
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
        data = toArrayBuffer(NodePB.Serialize(query, "Query"))

        length = data.byteLength
        finalArray = new Uint8Array length + 4
        (new DataView(finalArray.buffer)).setInt32(0, length, true)
        finalArray.set((new Uint8Array(data)), 4)

        @write finalArray.buffer

class TcpConnection extends Connection
    @isAvailable: -> testFor('net')

    constructor: (host, callback) ->
        unless TcpConnection.isAvailable()
            throw new RqlDriverError "TCP sockets are not available in this environment"

        super(host, callback)

        if @rawSocket?
            @close()

        net = require('net')
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
                arr = toArrayBuffer(buf)

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
                                @_data(toArrayBuffer(buf))

                            @emit 'connect'
                            return
                        else
                            @emit 'error', new RqlDriverError "Server dropped connection with message: \"" + status_str.trim() + "\""
                            return

            @rawSocket.on 'data', handshake_callback

        @rawSocket.on 'error', (args...) => @emit 'error', args...

        @rawSocket.on 'close', => @open = false; @emit 'close'

        setTimeout( (()=>
            if not handshake_complete
                @rawSocket.destroy()
                @emit 'error', new RqlDriverError "Handshake timedout"
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

bufferConcat = (buf1, buf2) ->
    view = new Uint8Array (buf1.byteLength + buf2.byteLength)
    view.set new Uint8Array(buf1), 0
    view.set new Uint8Array(buf2), buf1.byteLength
    view.buffer

toArrayBuffer = (node_buffer) ->
    # Convert from node buffer to array buffer
    arr = new Uint8Array new ArrayBuffer node_buffer.length
    for byte,i in node_buffer
        arr[i] = byte
    return arr.buffer


# AST classes

class TermBase
    constructor: ->
        self = (ar (field) -> self.getField(field))
        self.__proto__ = @.__proto__
        return self

    run: (connOrOptions, cb) ->
        useOutdated = undefined

        # Parse out run options from connOrOptions object
        if connOrOptions? and typeof(connOrOptions) is 'object' and not (connOrOptions instanceof Connection)
            useOutdated = !!connOrOptions.useOutdated
            noreply = !!connOrOptions.noreply
            for own key of connOrOptions
                unless key in ['connection', 'useOutdated', 'noreply']
                    throw new RqlDriverError "First argument to `run` must be an open connection or { connection: <connection>, useOutdated: <bool>, noreply: <bool>}."
            conn = connOrOptions.connection
        else
            useOutdated = null
            noreply = null
            conn = connOrOptions

        # This only checks that the argument is of the right type, connection
        # closed errors will be handled elsewhere
        unless conn instanceof Connection
            throw new RqlDriverError "First argument to `run` must be an open connection or { connection: <connection>, useOutdated: <bool>, noreply: <bool> }."

        # We only require a callback if noreply isn't set
        if not noreply and typeof(cb) isnt 'function'
            throw new RqlDriverError "Second argument to `run` must be a callback to invoke "+
                                     "with either an error or the result of the query."

        try
            conn._start @, cb, useOutdated, noreply
        catch e
            # It was decided that, if we can, we prefer to invoke the callback
            # with any errors rather than throw them as normal exceptions.
            # Thus we catch errors here and invoke the callback instead of
            # letting the error bubble up.
            if typeof(cb) is 'function'
                cb(e)
            else
                throw e

    toString: -> RqlQueryPrinter::printQuery(@)

class RDBVal extends TermBase
    eq: varar(1, null, (others...) -> new Eq {}, @, others...)
    ne: varar(1, null, (others...) -> new Ne {}, @, others...)
    lt: varar(1, null, (others...) -> new Lt {}, @, others...)
    le: varar(1, null, (others...) -> new Le {}, @, others...)
    gt: varar(1, null, (others...) -> new Gt {}, @, others...)
    ge: varar(1, null, (others...) -> new Ge {}, @, others...)

    not: ar () -> new Not {}, @

    add: varar(1, null, (others...) -> new Add {}, @, others...)
    sub: varar(1, null, (others...) -> new Sub {}, @, others...)
    mul: varar(1, null, (others...) -> new Mul {}, @, others...)
    div: varar(1, null, (others...) -> new Div {}, @, others...)
    mod: ar (other) -> new Mod {}, @, other

    append: ar (val) -> new Append {}, @, val
    prepend: ar (val) -> new Prepend {}, @, val
    difference: ar (val) -> new Difference {}, @, val
    setInsert: ar (val) -> new SetInsert {}, @, val
    setUnion: ar (val) -> new SetUnion {}, @, val
    setIntersection: ar (val) -> new SetIntersection {}, @, val
    setDifference: ar (val) -> new SetDifference {}, @, val
    slice: ar (left, right) -> new Slice {}, @, left, right
    skip: ar (index) -> new Skip {}, @, index
    limit: ar (index) -> new Limit {}, @, index
    getField: ar (field) -> new GetField {}, @, field
    contains: varar(1, null, (fields...) -> new Contains {}, @, fields...)
    insertAt: ar (index, value) -> new InsertAt {}, @, index, value
    spliceAt: ar (index, value) -> new SpliceAt {}, @, index, value
    deleteAt: varar(1, 2, (others...) -> new DeleteAt {}, @, others...)
    changeAt: ar (index, value) -> new ChangeAt {}, @, index, value
    indexesOf: ar (which) -> new IndexesOf {}, @, funcWrap(which)
    hasFields: varar(0, null, (fields...) -> new HasFields {}, @, fields...)
    withFields: varar(0, null, (fields...) -> new WithFields {}, @, fields...)
    keys: ar(-> new Keys {}, @)

    # pluck and without on zero fields are allowed
    pluck: (fields...) -> new Pluck {}, @, fields...
    without: (fields...) -> new Without {}, @, fields...

    merge: ar (other) -> new Merge {}, @, other
    between: aropt (left, right, opts) -> new Between opts, @, left, right
    reduce: aropt (func, base) -> new Reduce {base:base}, @, funcWrap(func)
    map: ar (func) -> new Map {}, @, funcWrap(func)
    filter: aropt (predicate, opts) -> new Filter opts, @, funcWrap(predicate)
    concatMap: ar (func) -> new ConcatMap {}, @, funcWrap(func)
    orderBy: varar(1, null, (fields...) -> new OrderBy {}, @, fields...)
    distinct: ar () -> new Distinct {}, @
    count: varar(0, 1, (fun...) -> new Count {}, @, fun...)
    union: varar(1, null, (others...) -> new Union {}, @, others...)
    nth: ar (index) -> new Nth {}, @, index
    match: ar (pattern) -> new Match {}, @, pattern
    isEmpty: ar () -> new IsEmpty {}, @
    groupedMapReduce: aropt (group, map, reduce, base) -> new GroupedMapReduce {base:base}, @, funcWrap(group), funcWrap(map), funcWrap(reduce)
    innerJoin: ar (other, predicate) -> new InnerJoin {}, @, other, predicate
    outerJoin: ar (other, predicate) -> new OuterJoin {}, @, other, predicate
    eqJoin: aropt (left_attr, right, opts) -> new EqJoin opts, @, left_attr, right
    zip: ar () -> new Zip {}, @
    coerceTo: ar (type) -> new CoerceTo {}, @, type
    typeOf: ar () -> new TypeOf {}, @
    update: aropt (func, opts) -> new Update opts, @, funcWrap(func)
    delete: aropt (opts) -> new Delete opts, @
    replace: aropt (func, opts) -> new Replace opts, @, funcWrap(func)
    do: ar (func) -> new FunCall {}, funcWrap(func), @
    default: ar (x) -> new Default {}, @, x

    or: varar(1, null, (others...) -> new Any {}, @, others...)
    and: varar(1, null, (others...) -> new All {}, @, others...)

    forEach: ar (func) -> new ForEach {}, @, funcWrap(func)

    groupBy: (attrs..., collector) ->
        unless collector? and attrs.length >= 1
            numArgs = attrs.length + (if collector? then 1 else 0)
            throw new RqlDriverError "Expected 2 or more argument(s) but found #{numArgs}."
        new GroupBy {}, @, attrs, collector

    info: ar () -> new Info {}, @
    sample: ar (count) -> new Sample {}, @, count

class DatumTerm extends RDBVal
    args: []
    optargs: {}

    constructor: (val) ->
        self = super()
        self.data = val
        return self

    compose: ->
        switch typeof @data
            when 'string'
                '"'+@data+'"'
            else
                ''+@data

    build: ->
        datum = {}
        if @data is null
            datum.type = "R_NULL"
        else
            switch typeof @data
                when 'number'
                    datum.type = "R_NUM"
                    datum.r_num = @data
                when 'boolean'
                    datum.type = "R_BOOL"
                    datum.r_bool = @data
                when 'string'
                    datum.type = "R_STR"
                    datum.r_str = @data
                else
                    throw new RqlDriverError "Cannot convert `#{@data}` to Datum."
        term =
            type: "DATUM"
            datum: datum
        return term

    deconstruct: (datum) ->
        switch datum.type
            when "R_NULL"
                null
            when "R_BOOL"
                datum.r_bool
            when "R_NUM"
                datum.r_num
            when "R_STR"
                datum.r_str
            when "R_ARRAY"
                DatumTerm::deconstruct dt for dt in datum.r_array
            when "R_OBJECT"
                obj = {}
                for pair in datum.r_object
                    obj[pair.key] = DatumTerm::deconstruct(pair.val)
                obj

translateOptargs = (optargs) ->
    result = {}
    for own key,val of optargs
        # We translate known two word opt-args to camel case for your convience
        key = switch key
            when 'primaryKey' then 'primary_key'
            when 'returnVals' then 'return_vals'
            when 'useOutdated' then 'use_outdated'
            when 'nonAtomic' then 'non_atomic'
            when 'cacheSize' then 'cache_size'
            else key

        if key is undefined or val is undefined then continue
        result[key] = rethinkdb.expr val
    return result

class RDBOp extends RDBVal
    constructor: (optargs, args...) ->
        self = super()
        self.args =
            for arg,i in args
                if arg isnt undefined
                    rethinkdb.expr arg
                else
                    throw new RqlDriverError "Argument #{i} to #{@st || @mt} may not be `undefined`."
        self.optargs = translateOptargs(optargs)
        return self

    build: ->
        term = {args:[], optargs:[]}
        term.type = @tt
        for arg in @args
            term.args.push(arg.build())
        for own key,val of @optargs
            pair =
                key: key
                val: val.build()
            term.optargs.push(pair)
        return term

    compose: (args, optargs) ->
        if @st
            return ['r.', @st, '(', intspallargs(args, optargs), ')']
        else
            if shouldWrap(@args[0])
                args[0] = ['r(', args[0], ')']
            return [args[0], '.', @mt, '(', intspallargs(args[1..], optargs), ')']

intsp = (seq) ->
    unless seq[0]? then return []
    res = [seq[0]]
    for e in seq[1..]
        res.push(', ', e)
    return res

kved = (optargs) ->
    ['{', intsp([k, ': ', v] for own k,v of optargs), '}']

intspallargs = (args, optargs) ->
    argrepr = []
    if args.length > 0
        argrepr.push(intsp(args))
    if Object.keys(optargs).length > 0
        if argrepr.length > 0
            argrepr.push(', ')
        argrepr.push(kved(optargs))
    return argrepr

shouldWrap = (arg) ->
    arg instanceof DatumTerm or arg instanceof MakeArray or arg instanceof MakeObject

class MakeArray extends RDBOp
    tt: "MAKE_ARRAY"
    st: '[...]' # This is only used by the `undefined` argument checker

    compose: (args) -> ['[', intsp(args), ']']

class MakeObject extends RDBOp
    tt: "MAKE_OBJ"
    st: '{...}' # This is only used by the `undefined` argument checker

    constructor: (obj) ->
        self = super({})
        self.optargs = {}
        for own key,val of obj
            if typeof val is 'undefined'
                throw new RqlDriverError "Object field '#{key}' may not be undefined"
            self.optargs[key] = rethinkdb.expr val
        return self

    compose: (args, optargs) -> kved(optargs)

class Var extends RDBOp
    tt: "VAR"
    compose: (args) -> ['var_'+args[0]]

class JavaScript extends RDBOp
    tt: "JAVASCRIPT"
    st: 'js'

class Json extends RDBOp
    tt: "JSON"
    st: 'json'

class UserError extends RDBOp
    tt: "ERROR"
    st: 'error'

class ImplicitVar extends RDBOp
    tt: "IMPLICIT_VAR"
    compose: -> ['r.row']

class Db extends RDBOp
    tt: "DB"
    st: 'db'

    tableCreate: aropt (tblName, opts) -> new TableCreate opts, @, tblName
    tableDrop: ar (tblName) -> new TableDrop {}, @, tblName
    tableList: ar(-> new TableList {}, @)

    table: aropt (tblName, opts) -> new Table opts, @, tblName

class Table extends RDBOp
    tt: "TABLE"
    st: 'table'

    get: ar (key) -> new Get {}, @, key

    getAll: (keysAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        keys = keysAndOpts

        # Look for opts dict
        perhapsOptDict = keysAndOpts[keysAndOpts.length - 1]
        if perhapsOptDict and
                ((perhapsOptDict instanceof Object) and not (perhapsOptDict instanceof TermBase))
            opts = perhapsOptDict
            keys = keysAndOpts[0...(keysAndOpts.length - 1)]

        new GetAll opts, @, keys...

    # For this function only use `exprJSON` rather than letting it default to regular
    # `expr`. This will attempt to serialize as much of the document as JSON as possible.
    # This behavior can be manually overridden with either direct JSON serialization
    # or ReQL datum serialization by first wrapping the argument with `r.expr` or `r.json`.
    insert: aropt (doc, opts) -> new Insert opts, @, rethinkdb.exprJSON(doc)
    indexCreate: varar(1, 2, (name, defun) ->
        if defun?
            new IndexCreate {}, @, name, funcWrap(defun)
        else
            new IndexCreate {}, @, name
        )
    indexDrop: ar (name) -> new IndexDrop {}, @, name
    indexList: ar () -> new IndexList {}, @

    compose: (args, optargs) ->
        if @args[0] instanceof Db
            [args[0], '.table(', args[1], ')']
        else
            ['r.table(', args[0], ')']

class Get extends RDBOp
    tt: "GET"
    mt: 'get'

class GetAll extends RDBOp
    tt: "GET_ALL"
    mt: 'getAll'

class Eq extends RDBOp
    tt: "EQ"
    mt: 'eq'

class Ne extends RDBOp
    tt: "NE"
    mt: 'ne'

class Lt extends RDBOp
    tt: "LT"
    mt: 'lt'

class Le extends RDBOp
    tt: "LE"
    mt: 'le'

class Gt extends RDBOp
    tt: "GT"
    mt: 'gt'

class Ge extends RDBOp
    tt: "GE"
    mt: 'ge'

class Not extends RDBOp
    tt: "NOT"
    mt: 'not'

class Add extends RDBOp
    tt: "ADD"
    mt: 'add'

class Sub extends RDBOp
    tt: "SUB"
    mt: 'sub'

class Mul extends RDBOp
    tt: "MUL"
    mt: 'mul'

class Div extends RDBOp
    tt: "DIV"
    mt: 'div'

class Mod extends RDBOp
    tt: "MOD"
    mt: 'mod'

class Append extends RDBOp
    tt: "APPEND"
    mt: 'append'

class Prepend extends RDBOp
    tt: "PREPEND"
    mt: 'prepend'

class Difference extends RDBOp
    tt: "DIFFERENCE"
    mt: 'difference'

class SetInsert extends RDBOp
    tt: "SET_INSERT"
    mt: 'setInsert'

class SetUnion extends RDBOp
    tt: "SET_UNION"
    mt: 'setUnion'

class SetIntersection extends RDBOp
    tt: "SET_INTERSECTION"
    mt: 'setIntersection'

class SetDifference extends RDBOp
    tt: "SET_DIFFERENCE"
    mt: 'setDifference'

class Slice extends RDBOp
    tt: "SLICE"
    mt: 'slice'

class Skip extends RDBOp
    tt: "SKIP"
    mt: 'skip'

class Limit extends RDBOp
    tt: "LIMIT"
    st: 'limit'

class GetField extends RDBOp
    tt: "GET_FIELD"
    st: '(...)' # This is only used by the `undefined` argument checker

    compose: (args) -> [args[0], '(', args[1], ')']

class Contains extends RDBOp
    tt: "CONTAINS"
    mt: 'contains'

class InsertAt extends RDBOp
    tt: "INSERT_AT"
    mt: 'insertAt'

class SpliceAt extends RDBOp
    tt: "SPLICE_AT"
    mt: 'spliceAt'

class DeleteAt extends RDBOp
    tt: "DELETE_AT"
    mt: 'deleteAt'

class ChangeAt extends RDBOp
    tt: "CHANGE_AT"
    mt: 'changeAt'

class Contains extends RDBOp
    tt: "CONTAINS"
    mt: 'contains'

class HasFields extends RDBOp
    tt: "HAS_FIELDS"
    mt: 'hasFields'

class WithFields extends RDBOp
    tt: "WITH_FIELDS"
    mt: 'withFields'

class Keys extends RDBOp
    tt: "KEYS"
    mt: 'keys'

class Pluck extends RDBOp
    tt: "PLUCK"
    mt: 'pluck'

class IndexesOf extends RDBOp
    tt: "INDEXES_OF"
    mt: 'indexesOf'

class Without extends RDBOp
    tt: "WITHOUT"
    mt: 'without'

class Merge extends RDBOp
    tt: "MERGE"
    mt: 'merge'

class Between extends RDBOp
    tt: "BETWEEN"
    mt: 'between'

class Reduce extends RDBOp
    tt: "REDUCE"
    mt: 'reduce'

class Map extends RDBOp
    tt: "MAP"
    mt: 'map'

class Filter extends RDBOp
    tt: "FILTER"
    mt: 'filter'

class ConcatMap extends RDBOp
    tt: "CONCATMAP"
    mt: 'concatMap'

class OrderBy extends RDBOp
    tt: "ORDERBY"
    mt: 'orderBy'

class Distinct extends RDBOp
    tt: "DISTINCT"
    mt: 'distinct'

class Count extends RDBOp
    tt: "COUNT"
    mt: 'count'

class Union extends RDBOp
    tt: "UNION"
    mt: 'union'

class Nth extends RDBOp
    tt: "NTH"
    mt: 'nth'

class Match extends RDBOp
    tt: "MATCH"
    mt: 'match'

class IsEmpty extends RDBOp
    tt: "IS_EMPTY"
    mt: 'isEmpty'

class GroupedMapReduce extends RDBOp
    tt: "GROUPED_MAP_REDUCE"
    mt: 'groupedMapReduce'

class GroupBy extends RDBOp
    tt: "GROUPBY"
    mt: 'groupBy'

class GroupBy extends RDBOp
    tt: "GROUPBY"
    mt: 'groupBy'

class InnerJoin extends RDBOp
    tt: "INNER_JOIN"
    mt: 'innerJoin'

class OuterJoin extends RDBOp
    tt: "OUTER_JOIN"
    mt: 'outerJoin'

class EqJoin extends RDBOp
    tt: "EQ_JOIN"
    mt: 'eqJoin'

class Zip extends RDBOp
    tt: "ZIP"
    mt: 'zip'

class CoerceTo extends RDBOp
    tt: "COERCE_TO"
    mt: 'coerceTo'

class TypeOf extends RDBOp
    tt: "TYPEOF"
    mt: 'typeOf'

class Info extends RDBOp
    tt: "INFO"
    mt: 'info'

class Sample extends RDBOp
    tt: "SAMPLE"
    mt: 'sample'

class Update extends RDBOp
    tt: "UPDATE"
    mt: 'update'

class Delete extends RDBOp
    tt: "DELETE"
    mt: 'delete'

class Replace extends RDBOp
    tt: "REPLACE"
    mt: 'replace'

class Insert extends RDBOp
    tt: "INSERT"
    mt: 'insert'

class DbCreate extends RDBOp
    tt: "DB_CREATE"
    st: 'dbCreate'

class DbDrop extends RDBOp
    tt: "DB_DROP"
    st: 'dbDrop'

class DbList extends RDBOp
    tt: "DB_LIST"
    st: 'dbList'

class TableCreate extends RDBOp
    tt: "TABLE_CREATE"
    mt: 'tableCreate'

class TableDrop extends RDBOp
    tt: "TABLE_DROP"
    mt: 'tableDrop'

class TableList extends RDBOp
    tt: "TABLE_LIST"
    mt: 'tableList'

class IndexCreate extends RDBOp
    tt: "INDEX_CREATE"
    mt: 'indexCreate'

class IndexDrop extends RDBOp
    tt: "INDEX_DROP"
    mt: 'indexDrop'

class IndexList extends RDBOp
    tt: "INDEX_LIST"
    mt: 'indexList'

class FunCall extends RDBOp
    tt: "FUNCALL"
    st: 'do' # This is only used by the `undefined` argument checker

    compose: (args) ->
        if args.length > 2
            ['r.do(', intsp(args[1..]), ', ', args[0], ')']
        else
            if shouldWrap(@args[1])
                args[1] = ['r(', args[1], ')']
            [args[1], '.do(', args[0], ')']

class Default extends RDBOp
    tt: "DEFAULT"
    mt: 'default'

class Branch extends RDBOp
    tt: "BRANCH"
    st: 'branch'

class Any extends RDBOp
    tt: "ANY"
    mt: 'or'

class All extends RDBOp
    tt: "ALL"
    mt: 'and'

class ForEach extends RDBOp
    tt: "FOREACH"
    mt: 'forEach'

class Func extends RDBOp
    tt: "FUNC"
    @nextVarId: 0

    constructor: (optargs, func) ->
        args = []
        argNums = []
        i = 0
        while i < func.length
            argNums.push Func.nextVarId
            args.push new Var {}, Func.nextVarId
            Func.nextVarId++
            i++

        body = func(args...)
        if body is undefined
            throw new RqlDriverError "Annonymous function returned `undefined`. Did you forget a `return`?"

        argsArr = new MakeArray({}, argNums...)
        return super(optargs, argsArr, body)

    compose: (args) ->
        ['function(', (Var::compose(arg) for arg in args[0][1...-1]), ') { return ', args[1], '; }']

class Asc extends RDBOp
    tt: "ASC"
    st: 'asc'

class Desc extends RDBOp
    tt: "DESC"
    st: 'desc'


# All top level exported functions

# Wrap a native JS value in an ReQL datum
rethinkdb.expr = ar (val) ->
    if val is undefined
        throw new RqlDriverError "Cannot wrap undefined with r.expr()."

    else if val instanceof TermBase
        val
    else if val instanceof Function
        new Func {}, val
    else if Array.isArray val
        new MakeArray {}, val...
    else if val == Object(val)
        new MakeObject val
    else
        new DatumTerm val

# Use r.json to serialize as much of the obect as JSON as is
# feasible to avoid doing too much protobuf serialization.
rethinkdb.exprJSON = ar (val) ->
    if isJSON(val)
        rethinkdb.json(JSON.stringify(val))
    else if (val instanceof TermBase)
        val
    else
        if Array.isArray(val)
            wrapped = []
        else
            wrapped = {}

        for k,v of val
            wrapped[k] = rethinkdb.exprJSON(v)
        rethinkdb.expr(wrapped)

# Is this JS value representable as JSON?
isJSON = (val) ->
    if (val instanceof TermBase)
        false
    else if (val instanceof Object)
        # Covers array case as well
        for k,v of val
            if not isJSON(v) then return false
        true
    else
        # Primitive types can always be represented as JSON
        true

rethinkdb.js = aropt (jssrc, opts) -> new JavaScript opts, jssrc

rethinkdb.json = ar (jsonsrc) -> new Json {}, jsonsrc

rethinkdb.error = varar 0, 1, (args...) -> new UserError {}, args...

rethinkdb.row = new ImplicitVar {}

rethinkdb.table = aropt (tblName, opts) -> new Table opts, tblName

rethinkdb.db = ar (dbName) -> new Db {}, dbName

rethinkdb.dbCreate = ar (dbName) -> new DbCreate {}, dbName
rethinkdb.dbDrop = ar (dbName) -> new DbDrop {}, dbName
rethinkdb.dbList = ar () -> new DbList {}

rethinkdb.tableCreate = aropt (tblName, opts) -> new TableCreate opts, tblName
rethinkdb.tableDrop = ar (tblName) -> new TableDrop {}, tblName
rethinkdb.tableList = ar () -> new TableList {}

rethinkdb.do = varar 1, null, (args...) ->
    new FunCall {}, funcWrap(args[-1..][0]), args[...-1]...

rethinkdb.branch = ar (test, trueBranch, falseBranch) -> new Branch {}, test, trueBranch, falseBranch

rethinkdb.count =              {'COUNT': true}
rethinkdb.sum   = ar (attr) -> {'SUM': attr}
rethinkdb.avg   = ar (attr) -> {'AVG': attr}

rethinkdb.asc = (attr) -> new Asc {}, attr
rethinkdb.desc = (attr) -> new Desc {}, attr

rethinkdb.eq = varar 2, null, (args...) -> new Eq {}, args...
rethinkdb.ne = varar 2, null, (args...) -> new Ne {}, args...
rethinkdb.lt = varar 2, null, (args...) -> new Lt {}, args...
rethinkdb.le = varar 2, null, (args...) -> new Le {}, args...
rethinkdb.gt = varar 2, null, (args...) -> new Gt {}, args...
rethinkdb.ge = varar 2, null, (args...) -> new Ge {}, args...
rethinkdb.or = varar 2, null, (args...) -> new Any {}, args...
rethinkdb.and = varar 2, null, (args...) -> new All {}, args...

rethinkdb.not = ar (x) -> new Not {}, x

rethinkdb.add = varar 2, null, (args...) -> new Add {}, args...
rethinkdb.sub = varar 2, null, (args...) -> new Sub {}, args...
rethinkdb.mul = varar 2, null, (args...) -> new Mul {}, args...
rethinkdb.div = varar 2, null, (args...) -> new Div {}, args...
rethinkdb.mod = ar (a, b) -> new Mod {}, a, b

rethinkdb.typeOf = ar (val) -> new TypeOf {}, val
rethinkdb.info = ar (val) -> new Info {}, val

# Export all names defined on rethinkdb
module.exports = rethinkdb
