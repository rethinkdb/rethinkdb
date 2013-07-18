rethinkdb = require(__dirname + '/ast')
net = require(__dirname + '/net')

# Add connect from net module
rethinkdb.connect = net.connect

module.exports = rethinkdb
