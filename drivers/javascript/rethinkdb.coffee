rethinkdb = require('./ast')
net = require('./net')

# Add connect from net module
rethinkdb.connect = net.connect

module.exports = rethinkdb
