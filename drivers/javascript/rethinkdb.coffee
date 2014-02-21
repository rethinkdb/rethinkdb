rethinkdb = require('./ast')
net = require('./net')
protobuf = require('./protobuf')
error = require('./errors')

# Add connect from net module
rethinkdb.connect = net.connect

# Add protobuf_implementation from the protobuf module
rethinkdb.protobuf_implementation = protobuf.protobuf_implementation

# Export Rql Errors
rethinkdb.Error = error

module.exports = rethinkdb
