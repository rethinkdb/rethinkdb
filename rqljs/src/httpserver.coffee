goog.provide('rethinkdb.HttpServer')

goog.require('rethinkdb.PbServer')

class RDBHttpServer
    isAvailable: -> (typeof require isnt 'undefined' and require('http'))

    constructor: (port) ->
        if not @isAvailable() then throw new ServerError "Http server unavailable"
        http = require('http')

        @pbserver = new RDBPbServer()
        @nextConnectionId = 1

        http.createServer((request, response) =>
            response.setHeader("Access-Control-Allow-Origin", "*")

            query = require('url').parse(request.url, true)

            match = query.pathname.match /^\/ajax\/reql\/(.*)/
            if match?
                switch match[1]
                    when "open-new-connection"
                        buf = new Buffer 4
                        buf.writeUInt32LE(@nextConnectionId++, 0)
                        response.end(buf)
                    when "close-connection"
                        response.end()
                    else
                        # Used in the main server to lookup a context object
                        # associated with this connection. We currently don't
                        # have any cross query state to manage so we'll ignore
                        # this parameter for now.
                        connId = query.query.conn_id
                        @handleQuery(request, response)

        ).listen(port)

    handleQuery: (request, response) ->
        readData = (buf) =>
            if buf.length < 4
                return 0

            length = buf.readUInt32LE 0
            total = length + 4

            # Does this buffer contain the full query?
            if buf.length <= total
                arraybufdata = new Uint8Array data
                result = @pbserver.execute arraybufdata
                response.write new Buffer result
                response.end()
                return total
            else
                # Wait for more data
                return 0

        data = new Buffer 0
        request.on 'data', (buf) =>
            data = Buffer.concat [data, buf]

            read = 1
            while read > 0
                read = readData data
                data = data.slice read


