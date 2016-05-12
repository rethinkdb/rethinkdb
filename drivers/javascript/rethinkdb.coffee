net = require('./net')
rethinkdb = require('./ast')
error = require('./errors')

# Add connect from net module
rethinkdb.connect = net.connect

# Export Reql Errors
rethinkdb.Error = error

# Re-export bluebird
rethinkdb._bluebird = require('bluebird')

module.exports = rethinkdb
module.exports.net = net
