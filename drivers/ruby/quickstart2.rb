# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts
$port_offset = ENV['PORT_OFFSET'].to_i
$c = r.connect(:host => 'localhost', :port => $port_offset + 28015, :db => 'test').repl
