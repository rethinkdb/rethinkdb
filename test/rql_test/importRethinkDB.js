var path = require('path');
var fs = require('fs');

// -- load rethinkdb from the proper location

rethinkdbLocation = process.env.JAVASCRIPT_DRIVER_DIR || path.join(__dirname, '..', '..', 'build', 'packages', 'js');
if (fs.existsSync(path.resolve(rethinkdbLocation, 'rethinkdb.js')) == false) {
    process.stdout.write('Could not locate the javascript drivers at the expected location: ' + rethinkdbLocation);
    process.exit(1);
}
var r = require(path.resolve(rethinkdbLocation, 'rethinkdb'));
console.log('Using RethinkDB client from: ' + rethinkdbLocation)

var bluebirdLocation = process.env.BLUEBIRD_DIR || path.join(rethinkdbLocation, 'node_modules', 'bluebird', 'js', 'main');
if (fs.existsSync(path.resolve(bluebirdLocation, 'bluebird.js')) == false) {
    process.stdout.write('Could not locate the bluebird module at the expected location: ' + bluebirdLocation);
    process.exit(1);
}
var Bluebird = require(path.resolve(bluebirdLocation, 'bluebird'));
console.log('Using bluebird from: ' + bluebirdLocation);

module.exports.r = r
module.exports.bluebird = Bluebird
