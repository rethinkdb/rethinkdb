import rethinkdb as r

# Open a connection to localhost:28015, with 'test' as the default
# database.
conn = r.connect()

# We must explicitly pass conn.
print "Tables:", r.table_list().run(conn)

# Start "repl" mode, which is useful for irb and small scripts.
conn.repl()

# Now we don't need to explicitly pass conn.  conn.repl() put conn in
# a global variable that run defaults to using.
print "Tables:", r.table_list().run()
