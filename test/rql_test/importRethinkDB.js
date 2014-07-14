
var path = require('path');
var fs = require('fs');

// -- load rethinkdb from the proper location

rethinkdbLocation = '';
try {
    rethinkdbLocation = process.env.JAVASCRIPT_DRIVER_DIR;
} catch(e) {
    dirPath = path.resolve(__dirname)
    while (dirPath != path.sep) {
        if (fs.existsSync(path.resolve(dirPath, 'drivers', 'javascript'))) {
            rethinkdbLocation = path.resolve(dirPath, 'build', 'packages', 'js');
            break;
        }
        dirPath = path.dirname(targetPath)
    }
}
if (fs.existsSync(path.resolve(rethinkdbLocation, 'rethinkdb.js')) == false) {
    process.stdout.write('Could not locate the javascript drivers at the expected location: ' + rethinkdbLocation);
    process.exit(1);
}
var r = require(path.resolve(rethinkdbLocation, 'rethinkdb'));
console.log('Using RethinkDB client from: ' + rethinkdbLocation)

module.exports.r = r
