# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
load 'rethinkdb.rb'
extend RethinkDB::Shortcuts
$port_offset = 32600#ENV['PORT_OFFSET'].to_i || 0 if not $port_offset
print "WARNING: No `PORT_OFFSET` environment variable, using 0\n" if $port_offset == 0
$c = RethinkDB::Connection.new('localhost', $port_offset + 28016, 'test')
