$LOAD_PATH.unshift('./rethinkdb')
load 'rethinkdb.rb'
extend RethinkDB::Shortcuts
port_offset = ENV['PORT_OFFSET'].to_i || 0 if not port_offset
print "WARNING: No `PORT_OFFSET` environment variable, using 0\n" if port_offset == 0
$c = RethinkDB::Connection.new('localhost', port_offset + 12346, 'test')
print "Loaded shortcut: r\n"
print "Connection: $c\n"
print "Examples: `r.list.run`, `$rdb.insert({:id => 0}).run`, `$rdb.run`\n"

