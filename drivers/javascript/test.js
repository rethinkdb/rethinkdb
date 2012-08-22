// Load text encoder, still haven't resolved this issue
var te = require('./encoding.js');
global.TextEncoder = te.TextEncoder;
global.TextDecoder = te.TextDecoder;

global.exit = function() {
    process.exit();
}

global.rethinkdb = require('./rethinkdb');
require('./rethinkdb/test.js');
