util = require('./util')
fs = require('fs')
node_protobuf = require('node-protobuf')

# Initialize message serializer with ReQL message descriptor file
desc = fs.readFileSync(__dirname + '/ql2.desc')
NodePB = new node_protobuf.Protobuf(desc)

module.exports.SerializeQuery = (query) ->
    util.toArrayBuffer(NodePB.Serialize(query, "Query"))

module.exports.ParseResponse = (data) ->
    NodePB.Parse(data, "Response")
