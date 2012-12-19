goog.provide('rethinkdb.TCPServer')

goog.require('rethinkdb.Errors')
goog.require('rethinkdb.PbServer')

class RDBTcpServer
    V0_1 = 0x3f61ba36

    isAvailable: -> (typeof require isnt 'undefined' and require('net'))

    constructor: (port) ->
        if not @isAvailable() then throw new ServerError "TCP server unavailable"
        net = require('net')

        @pbserver = new RDBPbServer()
        @server = net.createServer (socket) => @manageConnection socket
        @server.listen port, "localhost"
    
    manageConnection: (socket) ->
        data = null

        socket.on 'data', (buf) =>
            offset = 0
            while offset < buf.length
                if not data
                    # Start of a new query
                    magic = buf.readUint32LE(offset)
                    offset += 4
                    if magic isnt V0_1 then return

                    length = buf.readUint32LE(offset)
                    offset += 4

                    data = new Buffer(length)
                    data.have = 0

                toRead = buf.length - offset
                toRead = toRead < data.length ? data.length

                buf.copy(data, 0, offset, toRead)
                offset += toRead
                data.have += toRead

                if data.have is data.length
                    console.log "Full query"
                    arraybufdata = new Uint8Array data
                    data = null
                    socket.write @pbserver.execute arraybufdata
