#!/usr/bin/env node

var path = require('path');
var fs = require('fs');

// -- load rethinkdb from the proper location

rethinkdbLocation = process.env.JAVASCRIPT_DRIVER_DIR || path.join(__dirname, '..', '..', 'build', 'packages', 'js');
if (fs.existsSync(path.resolve(rethinkdbLocation, 'rethinkdb.js')) == false) {
    process.stdout.write('Could not locate the javascript drivers at the expected location: ' + rethinkdbLocation);
    process.exit(1);
}
var r        = require(path.resolve(rethinkdbLocation, 'rethinkdb'));
var protodef = require(path.resolve(rethinkdbLocation, 'proto-def'));
var net      = require(path.resolve(rethinkdbLocation, 'net'));

if (require.main === module) {
    banner = 'The RethinkDB driver has been imported as `r`, ';
    
    // setup default conn object
    var DRIVER_PORT = process.env.RDB_DRIVER_PORT || net.DEFAULT_PORT;
    var DRIVER_HOST = process.env.RDB_SERVER_HOST || net.DEFAULT_HOST;
    var conn = null;
    r.connect({'host':DRIVER_HOST, 'port':DRIVER_PORT})
    .then(function(c) {
        conn = c;
        banner += ' and a connection to ' + DRIVER_HOST + ':' + DRIVER_PORT + ' is available as `conn`.';
    }).error(function(err) {
        banner += ' but a default connection to ' + DRIVER_HOST + ':' + DRIVER_PORT + ' could not be made.';
    }).then(function() {
        // print the banner
        console.log(banner)
        
        // start the repl
        var repl = require('repl');
        var cli  = repl.start({ replMode: repl.REPL_MODE_MAGIC });
        cli.context.r = r;
        cli.context.rethindb = r;
        cli.context.protodef = protodef;
        if (conn) {
            cli.context.conn = conn;
        }
    });
    
} else {
    console.log('Using RethinkDB client from: ' + rethinkdbLocation)
    module.exports.r = r;
    module.exports.protodef = protodef;
}
    
