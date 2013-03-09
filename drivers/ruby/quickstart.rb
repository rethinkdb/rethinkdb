# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
load 'rethinkdb.rb'
extend RethinkDB::Shortcuts
port_offset = ENV['PORT_OFFSET'].to_i || 0 if not port_offset
print "WARNING: No `PORT_OFFSET` environment variable, using 0\n" if port_offset == 0
$c = RethinkDB::Connection.new('localhost', port_offset + 28015, 'test')
begin
  r.db('test').create_table('tbl').run
rescue
  # It don't matter to Jesus
end
$rdb = r.db('test').table('tbl')
print "Loaded shortcut: r\n"
print "Connection: $c, table: $rdb\n"
print "Examples: `r.list.run`, `$rdb.insert({:id => 0}).run`, `$rdb.run`\n"

