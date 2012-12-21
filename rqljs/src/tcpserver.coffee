goog.provide('rethinkdb.TCPServer')

goog.require('rethinkdb.Errors')
goog.require('rethinkdb.PbServer')

class RDBTcpServer
    isAvailable: -> (typeof require isnt 'undefined' and require('net'))

    constructor: (port) ->
        if not @isAvailable() then throw new ServerError "TCP server unavailable"
        net = require('net')

        @pbserver = new RDBPbServer()
        @server = net.createServer (socket) => @manageConnection socket
        @server.listen port, "localhost"
    
    manageConnection: (socket) ->
        # This function implements a state machine in its closure
        # for managing data delivered on the socket. Yeah, its ugly,
        # I too wish I could have coroutines.

        readData = null

        readMagic = (buf) =>
            magic = buf.readUInt32LE 0
            if magic is VersionDummy.Version.V0_1
                readData = readQuery
            return 4

        readQuery = (buf) =>
            if buf.length < 4
                return 0

            length = buf.readUInt32LE 0
            total = length + 4

            # Does this buffer contain the full query?
            if buf.length <= total
                arraybufdata = new Uint8Array data
                socket.write @pbserver.execute arraybufdata
                return total
            else
                # Wait for more data
                return 0

        readData = readMagic

        data = new Buffer 0
        socket.on 'data', (buf) =>
            data = Buffer.concat [data, buf]

            read = 1
            while read > 0
                read = readData data
                data = data.slice read

# Buffer.concat wasn't added until after my version of node
# so I'm just going to implement it here
Buffer.concat = (list, totalLength) ->
    if not totalLength?
        totalLength = list.reduce ((acc, buf) -> acc + buf.length), 0

    neu = new Buffer totalLength
    
    offset = 0
    for buf in list
        buf.copy neu, offset
        offset += buf.length

    return neu
