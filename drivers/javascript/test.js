HOST = process.argv[2] || 'newton';
PORT = parseInt(process.argv[3]) || 12346;

global.rethinkdb = require('./rethinkdb');
require('./test-driver');
require('./rethinkdb/test.js');
