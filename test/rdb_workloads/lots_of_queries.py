#!/usr/bin/python
# Copyright 2013 RethinkDB, all rights reserved.

# Environment variables:
# HOST: location of server (default = "localhost")
# PORT: port that server listens for RDB protocol traffic on (default = 14356)
# DB_NAME: database to look for test table in (default = "test")
# TABLE_NAME: table to run tests against (default = "test")
# NUM_QUERIES: the number of queries to run (default = 1000)

from rethinkdb import r
import os

num_queries = int(os.environ.get('NUM_QUERIES', 1000))
host_name = os.environ.get('HOST', 'localhost')
port_number = int(os.environ.get('PORT', 28015))
db_name = os.environ.get('DB_NAME', 'test')
table_name = os.environ.get('TABLE_NAME', 'test')

print "Connecting to %s:%s" % (host_name, port_number)
conn = r.connect(host_name, port_number)


# Create the database and table, if necessary.
try:
    conn.run(r.db_create(db_name))
except:
    pass

try:
    conn.run(db.table_create(table_name))
except:
    pass

pid = os.getpid()

db = r.db(db_name)
table = db.table(table_name)

for i in xrange(num_queries):
    conn.run(table.insert({'number': i, 'pid': pid}))


