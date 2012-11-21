# Local JavaScript server
class DemoServer
    constructor: ->
        #BC: The way you construct an empty object and then pass it into
        #    wrapper classes for each protobuf class is very weird.
        #    I'm going to invert this inverted structure so that the server
        #    executes each protobuf element and modifies its own state as it goes
        #@local_server = {}

        #BC: There is no reason to separate these into separate methods
        @descriptor = window.Query.getDescriptor()
        @serializer = new goog.proto2.WireFormatSerializer()

        #BC: Here we set up the internal state of the server in a way that allows
        #    people reading this constructor to understand how data in the server
        #    is represented.

        # Map of databases
        @dbs = {}

        # Add default database test
        @createDatabase 'test'

    createDatabase: (name) ->
        @dbs[name] = new RDBDatabase

    #BC: The server should log, yes, but abstracting log allows us to
    #    turn it off, redirect it from the console, add timestamps, etc.
    log: (msg) -> console.log msg

    print_all: ->
        console.log @local_server

    # Validate the length of the received buffer and return the stripped buffer
    validateBuffer: (buffer) ->
        #BC: js/cs uses camelCase by convention
        uint8Array = new Uint8Array buffer
        dataView = new DataView buffer
        expectedLength = dataView.getInt32(0, true)
        pb = uint8Array.subarray 4
        unless pb.length is expectedLength
            return pb
        else
            #BC: We need to agree on a way to do errors
            throw "PB buffer not of the correct length"

    deserializePB: (pbArray) ->
        @serializer.deserialize @descriptor, pbArray

    #BC: class methods need not be bound (fat arrow)
    execute: (data) ->
        query = @deserializePB @validateBuffer data
        @log 'Server: deserialized query'
        @log query

        #BC: Your Query class conflicts with the Query class in the protobuf 
        #    Be careful not to shadow names like this
        #    In any case, I don't this your query class is necessary
        #query = new Query data_query

        # Raw JSON result of the query
        result = @evaluateQuery query

        # Construct the response
        # ...
        response = new Response
        response.setToken query.getToken()

        @log 'Server: response'
        @log response

        # Serialize to protobuf format
        pbResponse = @serializer.serialize response
        length = pbResponse.byteLength

        # Add the length
        #BC: Use a dataview object to write binary representation of numbers
        #    into an array buffer
        finalPb = new Uint8Array(length + 4)
        dataView = new DataView finalPb.buffer
        dataView.setInt32 0, length, true

        finalPb.set pbResponse, 4

        @log 'Server: serialized response'
        @log finalPb
        return finalPb


window.DemoServer = DemoServer
