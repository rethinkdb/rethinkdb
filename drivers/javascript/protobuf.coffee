
# We can serialize and desierialize with protobufjs but prefer 'native-protobuf'
# (which uses node-protobuf) because it's faster.
pb = require('protobufjs')
native_pb = require('./native-protobuf')

# Initialize message classes with
builder = pb.protoFromFile(__dirname + '/ql2.proto')
Query = builder.result.Query
Response = builder.result.Response
Datum = builder.result.Datum

module.exports.SerializeQuery = (query) ->
    if native_pb.SerializeQuery?
        native_pb.SerializeQuery(query)
    else
        querypb = new Query(query)
        querypb.toArrayBuffer()

module.exports.ParseResponse = (data) ->
    if native_pb.ParseResponse?
        native_pb.ParseResponse(data)
    else
        Response.decode(data)

# Switch on the response type field of response
module.exports.ResponseTypeSwitch = (response, map, dflt) ->

    # One protobuf library gives us string type names, the
    # other type enum values. Handle both cases by converting
    # strings to enum values and switching based on that.
    type = response.type
    if typeof type is 'string'
        type = Response.ResponseType[type]

    # Reverse lookup this case in map
    for own type_str,type_val of Response.ResponseType
        if type is type_val
            if map[type_str]?
                return map[type_str]()
            else
                break

    # This case not covered in map, execute default case
    return dflt()

# Swith on the datum type field of datum
module.exports.DatumTypeSwitch = (datum, map, dflt) ->
    type = datum.type
    if typeof type is 'string'
        type = Datum.DatumType[type]

    for own type_str,type_val of Datum.DatumType
        if type is type_val
            if map[type_str]?
                return map[type_str]()
            else
                break

    return dflt()
