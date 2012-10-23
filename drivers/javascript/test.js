HOST = process.argv[2] || 'localhost';
PORT = parseInt(process.argv[3]) || 12346;

global.rethinkdb = require('./rethinkdb');
require('./test-driver');
require('./rethinkdb/test.js');
