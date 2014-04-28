net = require('./net')
rethinkdb = require('./ast')
error = require('./errors')

# Add connect from net module
rethinkdb.connect = net.connect

# Export Rql Errors
rethinkdb.Error = error

module.exports = rethinkdb
