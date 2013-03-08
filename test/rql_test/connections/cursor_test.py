from subprocess import Popen, call
from time import sleep
from random import randint
from os import putenv
from sys import path
path.append("../../../drivers/python2")

import rethinkdb as r

# Run the server in default configuration
cpp_server = Popen(['../../../build/debug_clang/rethinkdb', '--port-offset=2348'])
sleep(0.1)
print ''

try:
    c = r.connect(port=30363)

    r.db('test').table_create('test').run(c)
    tbl = r.table('test')

    num_rows = randint(1111, 2222)

    print "Inserting %d rows" % num_rows
    tbl.insert([{'id':i} for i in xrange(0, num_rows)]).run(c)
    print "Done\n"

    print "Running Python"
    call(["python", "cursor.py", str(num_rows)])
    print ''

    print "Running JS"
    call(["node", "cursor.js", str(num_rows)])
    print ''

finally:

    cpp_server.terminate()
    call(['rm', '-r', 'rethinkdb_data'])
    sleep(0.1)
