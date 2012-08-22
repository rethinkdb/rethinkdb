// Load text encoder, still haven't resolved this issue
var te = require('./encoding.js');
global.TextEncoder = te.TextEncoder;
global.TextDecoder = te.TextDecoder;

global.rethinkdb = require('./rethinkdb');
var tests = require('./rethinkdb');
