util = require('./util')
fs = require('fs')
node_protobuf = require('node-protobuf')

# Initialize message serializer with ReQL message descriptor file
desc = fs.readFileSync(__dirname + '/ql2.desc')
if node_protobuf.Protobuf?
    # For node-protobuf < 1.1.0
    NodePB = new node_protobuf.Protobuf(desc)
else
    # For node-protobuf >= 1.1.0
    NodePB = new node_protobuf(desc)

module.exports.SerializeQuery = (query) ->
    NodePB.Serialize(query, "Query")

module.exports.ParseResponse = (data) ->
    NodePB.Parse(data, "Response")
