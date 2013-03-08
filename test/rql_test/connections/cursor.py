###
# Tests the driver cursor API
###

from subprocess import Popen, call
from time import sleep
from sys import path
path.append("../../../drivers/python2")

import rethinkdb as r

# Run servers in default configuration
cpp_server = Popen(['../../../build/release_clang/rethinkdb', '--port-offset=2348'])
sleep(0.1)

try:
    c = r.connect(port=30363)

    r.db('test').table_create('test').run(c)
    tbl = r.table('test')

    num_rows = 2621

    tbl.insert([{'id':i} for i in xrange(0, num_rows)]).run(c)

    cur = tbl.run(c)

    i = 0
    for row in cur:
        i += 1

    if not i == num_rows:
        raise "Test failed: expected %d rows but found %d." % (num_rows, i)
    else:
        print "Test passed!"

finally:
    cpp_server.terminate()
    call(['rm', '-r', 'rethinkdb_data'])
    sleep(0.1)
