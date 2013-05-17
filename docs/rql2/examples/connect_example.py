import rethinkdb as r

# A fresh RethinkDB instance comes with an empty database named
# 'test' created for you.

# Open a connection with 'test' as the default database.
conn = r.connect('localhost', 28015, 'test')

# Create a second database, 'test_two'.
r.db_create('test_two').run(conn)

print "Databases: ", r.db_list().run(conn)

# Create a table in conn's default database, 'test'.
r.table_create('alice').run(conn)

# Create another table, explicitly specifying the database 'test'.
r.db('test').table_create('bob').run(conn)

# Create a third table, explicitly specifynig the database 'test_two'.
r.db('test_two').table_create('charlie').run(conn)

# Print the tables of conn's default database, 'test'.
print "Tables in 'test': ", r.table_list().run(conn)
# Print the tables of 'test_two'.
print "Tables in 'test_two': ", r.db('test_two').table_list().run(conn)

# Clean up our work.  (This example code is not exception-safe.)

# Drop the database 'test_two' and ALL of its tables.
r.db_drop('test_two').run(conn)

# Drop the tables 'alice' and 'bob' from conn's default database,
# which is 'test'.
r.table_drop('alice').run(conn)
r.table_drop('bob').run(conn)

conn.close()

