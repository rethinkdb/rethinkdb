import rethinkdb as r

def setup():
    conn = r.connect()
    r.db_create('reality').run()
    r.db_create('fiction').run()
    r.db('reality').table_create('states').run()
    r.db('fiction').table_create('states').run()
    conn.close()

def cleanup()
    conn = r.connect()
    r.db_drop('reality')
    r.db_drop('fiction')
    conn.close()

setup()

# <BEGIN EXAMPLE>

conn = r.connect(db='reality')

# Open a connection with the default database set to 'reality'
conn = r.connect(db='real')

# Insert a document into the table 'states' in the 'reality' database.
r.table('states').insert({'name': 'New York', 'code': 'NY'}).run(conn)

# Change the default database to 'fantasy'.
conn.use('fiction')

# Insert a document into the entirely different table 'states' in
# the 'fiction' database.
r.table('states').insert({'name': 'Cascadia', 'code': 'CS'}).run(conn)

# <END EXAMPLE>

conn.close()

cleanup()
