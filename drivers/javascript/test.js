// Copyright 2010-2012 RethinkDB, all rights reserved.
HOST = process.argv[2] || 'localhost';
PORT = parseInt(process.argv[3]) || 28015;

global.rethinkdb = require('./rethinkdb');
require('./test-driver');
require('./rethinkdb/test.js');
