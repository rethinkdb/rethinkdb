var path = require('path');
var fs = require('fs');

// -- load rethinkdb from the proper location

rethinkdbLocation = process.env.JAVASCRIPT_DRIVER_DIR || path.join(__dirname, '..', '..', 'build', 'packages', 'js');
if (fs.existsSync(path.resolve(rethinkdbLocation, 'rethinkdb.js')) == false) {
    process.stdout.write('Could not locate the javascript drivers at the expected location: ' + rethinkdbLocation);
    process.exit(1);
}
var r = require(path.resolve(rethinkdbLocation, 'rethinkdb'));
var protodef = require(path.resolve(rethinkdbLocation, 'proto-def'));
console.log('Using RethinkDB client from: ' + rethinkdbLocation)

module.exports.r = r
module.exports.protodef = protodef
