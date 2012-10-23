HOST = process.argv[2] || 'localhost';
PORT = parseInt(process.argv[3]) || 28015;

global.rethinkdb = require('./rethinkdb');
require('./test-driver');
require('./rethinkdb/test.js');
