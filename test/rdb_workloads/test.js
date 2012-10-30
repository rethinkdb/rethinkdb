// Copyright 2010-2012 RethinkDB, all rights reserved.
HOST = process.argv[2];
PORT = process.argv[3];

global.rethinkdb = require('../../drivers/javascript/rethinkdb');
require('../../drivers/javascript/test-driver');
require('../../drivers/javascript/rethinkdb/test.js');
