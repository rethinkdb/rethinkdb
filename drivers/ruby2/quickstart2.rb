# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
load 'rethinkdb.rb'
extend RethinkDB::Shortcuts
$port_offset = 32600
$c = RethinkDB::Connection.new('localhost', $port_offset + 28016, 'test')
$rdb = r.db('test').table('test')
