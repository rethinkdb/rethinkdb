# Local JavaScript server
class DemoServer
    constructor: ->
        @local_server = {}
        @set_descriptor()
        @set_serializer()

    print_all: =>
        console.log @local_server

    set_descriptor: =>
        @descriptor = window.Query.getDescriptor()
    set_serializer: =>
        @serializer = new goog.proto2.WireFormatSerializer()

    buffer2pb: (buffer) ->
        expanded_array = new Uint8Array buffer
        return expanded_array.subarray 4

    pb2query: (intarray) =>
        @serializer.deserialize @descriptor, intarray

    execute: (data) =>
        data_query = @pb2query @buffer2pb data
        console.log 'Server: deserialized query'
        console.log data_query
        query = new Query data_query
        response = query.evaluate @local_server
        if response?
            response.setToken data_query.getToken()
        console.log 'Server: response'
        console.log response

        serialized_response = @serializer.serialize response
        length = serialized_response.byteLength
        final_response = new Uint8Array(length + 4)
        final_response[0] = length%256
        final_response[1] = Math.floor(length/256)
        final_response[2] = Math.floor(length/(256*256))
        final_response[3] = Math.floor(length/(256*256*256))
        final_response.set serialized_response, 4
        console.log 'Server: serialized response'
        console.log final_response
        return final_response


window.DemoServer = DemoServer
