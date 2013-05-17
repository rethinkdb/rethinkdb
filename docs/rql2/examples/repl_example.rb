require 'rethinkdb'
include RethinkDB::Shortcuts

# Open a connection to localhost:28015, with 'test' as the default
# database.
conn = r.connect()

# We must explicitly pass conn.
puts "Tables: [" + r.table_list().run(conn).join(', ') + "]"

# Start "repl" mode, which is useful for irb and small scripts.
conn.repl()

# Now we don't need to explicitly pass conn.  conn.repl() put conn in
# a global variable that the 'run' method uses by default.
puts "Tables: [" + r.table_list().run.join(', ') + "]"
