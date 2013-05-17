import rethinkdb as r

# Open a connection.
conn = r.connect('localhost', 28015, 'test')

# Create a table and insert three rows.
r.table_create('tristate_area').run(conn)
r.table('tristate_area').insert([{'name': 'Pennsylvania', 'code': 'PA'},
                                 {'name': 'New Jersey', 'code': 'NJ'},
                                 {'name': 'Delaware', 'code': 'DE'}]).run(conn)

# Make a cursor containing every row of the table and destructively
# iterate through its rows.
cursor = r.table('tristate_area').run(conn)
for row in cursor:
    print row

# Cleanup.  (Exception safety is not a part of this example!)
r.table_drop('tristate_area').run(conn)
conn.close()

